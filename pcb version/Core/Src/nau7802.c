/*
 * nau7802.c
 *
 *  Created on: Sep 15, 2026
 *      Author: mfdal
 */

#include "main.h"

extern I2C_HandleTypeDef hi2c1;

#define NAU7802_ADDRESS     0x2A

#define NAU7802_TIMEOUT_MS  500

/* register maps */

#define NAU7802_PU_CTRL     0x00
#define NAU7802_CTRL1       0x01
#define NAU7802_CTRL2       0x02
#define NAU7802_ADCO_B2     0x12
#define NAU7802_ADCO_B1     0x13
#define NAU7802_ADCO_B0     0x14
#define NAU7802_OTP_B1      0x15
#define NAU7802_ADC         0x15
#define NAU7802_OTP_B0      0x16
#define NAU7802_PGA         0x1B
#define NAU7802_PWR_CTRL    0x1C
#define NAU7802_REV_ID      0x1F

#define NAU7802_LDO_3V0     0x5
#define NAU7802_LDO_2V7     0x6
#define NAU7802_LDO_2V4     0x7

#define NAU7802_GAIN_X1     0x0
#define NAU7802_GAIN_X2     0x1
#define NAU7802_GAIN_X4     0x2
#define NAU7802_GAIN_X8     0x3
#define NAU7802_GAIN_X16    0x4
#define NAU7802_GAIN_X32    0x5
#define NAU7802_GAIN_X64    0x6
#define NAU7802_GAIN_X128   0x7

#define NAU7802_RATE_10SPS  0x0
#define NAU7802_RATE_20SPS  0x1
#define NAU7802_RATE_40SPS  0x2
#define NAU7802_RATE_80SPS  0x3
#define NAU7802_RATE_320SPS 0x7

#define NAU7802_CAL_INTERN  0x0
#define NAU7802_CAL_OFFSET  0x2
#define NAU7802_CAL_GAIN    0x3

/* end register defs */

/* prototypes */

HAL_StatusTypeDef NAU7802_ReadReg(uint8_t reg, uint8_t *value);
HAL_StatusTypeDef NAU7802_WriteReg(uint8_t reg, uint8_t value);
HAL_StatusTypeDef NAU7802_UpdateReg(uint8_t reg_addr, uint8_t mask,
		uint8_t value);

static uint8_t NAU7802_IsPowerReady();
static uint8_t NAU7802_IsCalibrationDone();

uint8_t NAU7802_Reset(void);
uint8_t NAU7802_Init(void);

uint8_t NAU7802_PowerUp(void);
uint8_t NAU7802_PowerDown(void);

/* helpers */

HAL_StatusTypeDef NAU7802_ReadReg(uint8_t reg, uint8_t *value) {
	return HAL_I2C_Mem_Read(&hi2c1,
	NAU7802_ADDRESS << 1, reg,
	I2C_MEMADD_SIZE_8BIT, value, 1,
	HAL_MAX_DELAY);
}

HAL_StatusTypeDef NAU7802_WriteReg(uint8_t reg, uint8_t value) {
	return HAL_I2C_Mem_Write(&hi2c1,
	NAU7802_ADDRESS << 1, reg,
	I2C_MEMADD_SIZE_8BIT, &value, 1,
	HAL_MAX_DELAY);
}

HAL_StatusTypeDef NAU7802_UpdateReg(uint8_t reg_addr, uint8_t mask,
		uint8_t value) {
	uint8_t reg;

	if (NAU7802_ReadReg(reg_addr, &reg) != HAL_OK)
		return HAL_ERROR;

	reg = (reg & ~mask) | (value & mask);

	return NAU7802_WriteReg(reg_addr, reg);
}

static uint8_t NAU7802_WaitPowerReady(uint32_t timeout_ms) {
	uint32_t start = HAL_GetTick();

	while (!NAU7802_IsPowerReady()) {
		if ((HAL_GetTick() - start) >= timeout_ms)
			return 1;   // timed out (or I2C is failing)
		HAL_Delay(1);
	}
	return 0;
}

/* main doodles */

uint8_t NAU7802_CalibrateInternal(void) {
	// set cal type
	if (NAU7802_UpdateReg(NAU7802_CTRL2, 0x3, NAU7802_CAL_INTERN) != HAL_OK)
		return 1;

	if (NAU7802_UpdateReg(NAU7802_CTRL2, (1u << 2), (1u << 2)) != HAL_OK)
		return 1;

	// wait for it
	uint32_t start = HAL_GetTick();
	while (!NAU7802_IsCalibrationDone()) {
		if ((HAL_GetTick() - start) >= 1000) // has one second to calibrate
			return 1;   // timed out (or I2C is failing)
		HAL_Delay(1);
	}

	// now did cal succeed?
	uint8_t reg;

	if (NAU7802_ReadReg(NAU7802_CTRL2, &reg) != HAL_OK)
		return 1;

	return (reg >> 3) & 1;
}

uint8_t NAU7802_Init(void) {
	// returns 0 on success, 1 on fail

	if (NAU7802_Reset())
		return 1;

	if (NAU7802_PowerUp())
		return 1;

	// LDO voltage
	if (NAU7802_UpdateReg(NAU7802_CTRL1, (0x7 << 3), NAU7802_LDO_3V0 << 3)
			!= HAL_OK)
		return 1;

	// LDO source
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 7), 1 << 7) != HAL_OK)
		return 1;

	// gain = 128x
	if (NAU7802_UpdateReg(NAU7802_CTRL1, 0x7, NAU7802_GAIN_X128) != HAL_OK)
		return 1;

	// 10 samples per second
	if (NAU7802_UpdateReg(NAU7802_CTRL2, (0x7 << 4), NAU7802_RATE_10SPS << 4)
			!= HAL_OK)
		return 1;

	// adc chop disable
	if (NAU7802_UpdateReg(NAU7802_ADC, (0x3 << 4), 0x3 << 4) != HAL_OK)
		return 1;

	// LDO mode to low esr caps
	if (NAU7802_UpdateReg(NAU7802_PGA, (1 << 6), 0) != HAL_OK)
		return 1;

	// set channel 1 active
	if (NAU7802_UpdateReg(NAU7802_CTRL2, (1 << 7), 0 << 7) != HAL_OK)
		return 1;

	// enable stabilizer cap (only use with one channel)
	if (NAU7802_UpdateReg(NAU7802_PWR_CTRL, (1 << 7), 0x1 << 7) != HAL_OK)
		return 1;

	return NAU7802_CalibrateInternal();
}

uint8_t NAU7802_Reset(void) {
	// returns 0 on success, 1 on fail

	// assert register reset
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 0), (1 << 0)) != HAL_OK)
		return 1;

	HAL_Delay(5);

	// release reset
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 0), 0) != HAL_OK)
		return 1;

	// power on digital
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 1), (1 << 1)) != HAL_OK)
		return 1;

	HAL_Delay(5);

	return 0;
}

static uint8_t NAU7802_IsPowerReady(void) {
	uint8_t reg;

	if (NAU7802_ReadReg(NAU7802_PU_CTRL, &reg) != HAL_OK)
		return 0;

	return (reg >> 3) & 1;
}

static uint8_t NAU7802_IsCalibrationDone(void) {
	uint8_t reg;

	if (NAU7802_ReadReg(NAU7802_CTRL2, &reg) != HAL_OK)
		return 0;

	return !((reg >> 2) & 1);
}

uint8_t NAU7802_PowerUp(void) {
	// power on digital
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 1), (1 << 1)) != HAL_OK)
		return 1;

	// power on analog
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 2), (1 << 2)) != HAL_OK)
		return 1;

	// wait for power up
	if (NAU7802_WaitPowerReady(100))
		return 1;

	// start acquisition
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 4), (1 << 4)) != HAL_OK)
		return 1;

	return 0;
}

uint8_t NAU7802_PowerDown(void) {
	// power down analog
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 2), 0) != HAL_OK)
		return 1;

	// power down digital
	if (NAU7802_UpdateReg(NAU7802_PU_CTRL, (1 << 1), 0) != HAL_OK)
		return 1;

	HAL_Delay(10); // let systems power off

	return 0;
}

HAL_StatusTypeDef NAU7802_DataReady(uint8_t *ready) {
	uint8_t reg;

	if (HAL_I2C_Mem_Read(&hi2c1,
	NAU7802_ADDRESS << 1,
	NAU7802_PU_CTRL,
	I2C_MEMADD_SIZE_8BIT, &reg, 1, 100) != HAL_OK) {
		return HAL_ERROR;
	}

	*ready = (reg & (1u << 5)) != 0;
	return HAL_OK;
}

HAL_StatusTypeDef NAU7802_ReadADC(int32_t *value) {
	uint8_t data[3];

	if (HAL_I2C_Mem_Read(&hi2c1,
	NAU7802_ADDRESS << 1,
	NAU7802_ADCO_B2,
	I2C_MEMADD_SIZE_8BIT, data, 3,
	HAL_MAX_DELAY) != HAL_OK) {
		return HAL_ERROR;
	}

	*value = ((int32_t) data[0] << 16) | ((int32_t) data[1] << 8) | data[2];

	// Sign extend 24-bit value
	if (*value & 0x800000)
		*value |= 0xFF000000;

	return HAL_OK;
}

HAL_StatusTypeDef NAU7802_ReadADCBlocking(int32_t *value) {
	uint32_t start = HAL_GetTick();
	uint8_t ready = 0;

	while (!ready) {
		if (NAU7802_DataReady(&ready) != HAL_OK)
			return HAL_ERROR;
		if (!ready && (HAL_GetTick() - start) > NAU7802_TIMEOUT_MS)
			return HAL_TIMEOUT;
		if (!ready)
			HAL_Delay(1);
	}

	return NAU7802_ReadADC(value);
}
