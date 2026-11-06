/*
 * power.c
 *
 *  Created on: Sep 24, 2026
 *      Author: mfdal
 */

#include "main.h"
#include "nau7802.h"
#include "fram.h"
#include "st7735.h"
#include "gfx.h"
#include "rfm95.h"

#define SLEEP_TIME 20

#define SPI_SCK_GPIO_Port   GPIOA
#define SPI_SCK_Pin         GPIO_PIN_5
#define SPI_MISO_GPIO_Port  GPIOA
#define SPI_MISO_Pin        GPIO_PIN_6
#define SPI_MOSI_GPIO_Port  GPIOA
#define SPI_MOSI_Pin        GPIO_PIN_7

extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi1;

void SystemClock_Config(void);
void MX_SPI1_Init(void);

void PowerOnDisplay(void) {
	HAL_GPIO_WritePin(TFT_PWR_GPIO_Port, TFT_PWR_Pin, GPIO_PIN_SET);

	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_SET);

	HAL_Delay(5);

	ST7735_Init();
	ST7735_Fill(ST7735_BLACK);
}

void PowerOffDisplay(void) {
	HAL_GPIO_WritePin(TFT_CS_GPIO_Port, TFT_CS_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(TFT_DC_GPIO_Port, TFT_DC_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(TFT_RST_GPIO_Port, TFT_RST_Pin, GPIO_PIN_RESET);

	HAL_GPIO_WritePin(TFT_PWR_GPIO_Port, TFT_PWR_Pin, GPIO_PIN_RESET);
}

static void SpiPinsToSleep(void) {
	GPIO_InitTypeDef g = { 0 };
	g.Pull = GPIO_NOPULL;
	g.Speed = GPIO_SPEED_FREQ_LOW;

	/* SCK, MOSI: driven low (the display's rail is off, and the FRAM/RFM95 don't care) */
	HAL_GPIO_WritePin(SPI_SCK_GPIO_Port, SPI_SCK_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SPI_MOSI_GPIO_Port, SPI_MOSI_Pin, GPIO_PIN_RESET);
	g.Mode = GPIO_MODE_OUTPUT_PP;
	g.Pin = SPI_SCK_Pin;
	HAL_GPIO_Init(SPI_SCK_GPIO_Port, &g);
	g.Pin = SPI_MOSI_Pin;
	HAL_GPIO_Init(SPI_MOSI_GPIO_Port, &g);

	/* MISO: the FRAM and RFM95 tristate it when deselected, so don't leave a live digital input floating */
	g.Mode = GPIO_MODE_ANALOG;
	g.Pin = SPI_MISO_Pin;
	HAL_GPIO_Init(SPI_MISO_GPIO_Port, &g);
}

void EnterSleep(void) {
	/* Turn off anything that should not run during STOP2 */
	PowerOffDisplay();

	NAU7802_PowerDown();

	lora_sleep();

	FRAM_Sleep();

	/* Stop SPI completely */
	HAL_SPI_DeInit(&hspi1);
	__HAL_RCC_SPI1_CLK_DISABLE();
	SpiPinsToSleep();

	/* Stop SysTick */
	HAL_SuspendTick();

	/* Disable debugger in STOP mode */
	//HAL_DBGMCU_DisableDBGStopMode();
	/* Set RTC wake-up timer */
	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc,
	SLEEP_TIME,
	RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);

	/* Enter STOP2 */
	HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
}

void PowerUp(void) {
	/* Restore system clock */
	SystemClock_Config();

	/* Resume SysTick */
	HAL_ResumeTick();

	/* Disable the RTC wake-up timer */
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

	/* Reinitialize SPI */
	MX_SPI1_Init();

	FRAM_Wake();

	NAU7802_PowerUp();

	lora_standby();
}
