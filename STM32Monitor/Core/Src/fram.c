/*
 * fram.c
 *
 *  Created on: Sep 24, 2026
 *      Author: mfdal
 */


#include "fram.h"

extern SPI_HandleTypeDef hspi1;

/* MB85RS1MT commands */
#define FRAM_CMD_WREN      0x06
#define FRAM_CMD_WRDI      0x04
#define FRAM_CMD_RDSR      0x05
#define FRAM_CMD_WRSR      0x01
#define FRAM_CMD_READ      0x03
#define FRAM_CMD_WRITE     0x02
#define FRAM_CMD_SLEEP     0xB9

#define FRAM_STATUS_WEL    0x02

static void FRAM_CS_Low(void)
{
    HAL_GPIO_WritePin(FERAM_CS_GPIO_Port,
                      FERAM_CS_Pin,
                      GPIO_PIN_RESET);
}

static void FRAM_CS_High(void)
{
    HAL_GPIO_WritePin(FERAM_CS_GPIO_Port,
                      FERAM_CS_Pin,
                      GPIO_PIN_SET);
}

static HAL_StatusTypeDef FRAM_SendAddress(uint32_t address)
{
    uint8_t addr[3];

    addr[0] = (uint8_t)(address >> 16);
    addr[1] = (uint8_t)(address >> 8);
    addr[2] = (uint8_t)(address);

    return HAL_SPI_Transmit(&hspi1, addr, 3, HAL_MAX_DELAY);
}

HAL_StatusTypeDef FRAM_Init(void)
{
    /*
     * Make sure CS starts inactive.
     */
    FRAM_CS_High();

    return HAL_OK;
}

HAL_StatusTypeDef FRAM_WriteEnable(void)
{
    uint8_t command = FRAM_CMD_WREN;

    FRAM_CS_Low();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1,
                         &command,
                         1,
                         HAL_MAX_DELAY);

    FRAM_CS_High();

    return status;
}

HAL_StatusTypeDef FRAM_Sleep(void)
{
	uint8_t command = FRAM_CMD_SLEEP;

	FRAM_CS_Low();

	HAL_StatusTypeDef status =
		HAL_SPI_Transmit(&hspi1,
						 &command,
						 1,
						 HAL_MAX_DELAY);

	FRAM_CS_High();

	return status;
}

HAL_StatusTypeDef FRAM_ReadStatus(uint8_t *status)
{
    uint8_t command = FRAM_CMD_RDSR;
    uint8_t dummy = 0xFF;

    FRAM_CS_Low();

    HAL_StatusTypeDef result =
        HAL_SPI_Transmit(&hspi1,
                         &command,
                         1,
                         HAL_MAX_DELAY);

    if (result == HAL_OK)
    {
        result = HAL_SPI_TransmitReceive(&hspi1,
                                         &dummy,
                                         status,
                                         1,
                                         HAL_MAX_DELAY);
    }

    FRAM_CS_High();

    return result;
}

HAL_StatusTypeDef FRAM_Read(uint32_t address,
                            uint8_t *data,
                            uint16_t length)
{
    if (data == NULL || length == 0)
        return HAL_ERROR;

    if (address >= FRAM_SIZE ||
        address + length > FRAM_SIZE)
        return HAL_ERROR;

    uint8_t command = FRAM_CMD_READ;

    FRAM_CS_Low();

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit(&hspi1,
                         &command,
                         1,
                         HAL_MAX_DELAY);

    if (status == HAL_OK)
    {
        status = FRAM_SendAddress(address);
    }

    if (status == HAL_OK)
    {
        status = HAL_SPI_Receive(&hspi1,
                                 data,
                                 length,
                                 HAL_MAX_DELAY);
    }

    FRAM_CS_High();

    return status;
}

HAL_StatusTypeDef FRAM_Write(uint32_t address,
                             const uint8_t *data,
                             uint16_t length)
{
    if (data == NULL || length == 0)
        return HAL_ERROR;

    if (address >= FRAM_SIZE ||
        address + length > FRAM_SIZE)
        return HAL_ERROR;

    HAL_StatusTypeDef status;

    /*
     * Set the Write Enable Latch.
     */
    status = FRAM_WriteEnable();

    if (status != HAL_OK)
        return status;

    /*
     * WRITE command + 24-bit address + data.
     */
    uint8_t command = FRAM_CMD_WRITE;

    FRAM_CS_Low();

    status = HAL_SPI_Transmit(&hspi1,
                             &command,
                             1,
                             HAL_MAX_DELAY);

    if (status == HAL_OK)
    {
        status = FRAM_SendAddress(address);
    }

    if (status == HAL_OK)
    {
        status = HAL_SPI_Transmit(&hspi1,
                                  (uint8_t *)data,
                                  length,
                                  HAL_MAX_DELAY);
    }

    FRAM_CS_High();

    return status;
}
