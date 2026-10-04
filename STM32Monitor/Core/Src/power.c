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

#define SLEEP_TIME 2

extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi1;

void SystemClock_Config(void);
void MX_SPI1_Init(void);

void EnterSleep(void)
{
    /* Turn off anything that should not run during STOP2 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    NAU7802_PowerDown();

    FRAM_Sleep();

    /* Stop SPI completely */
    HAL_SPI_DeInit(&hspi1);
    __HAL_RCC_SPI1_CLK_DISABLE();

    /* Stop SysTick */
    HAL_SuspendTick();

    /* Disable debugger in STOP mode */
    //HAL_DBGMCU_DisableDBGStopMode();

    /* Set RTC wake-up timer */
    HAL_RTCEx_SetWakeUpTimer_IT(
        &hrtc,
        SLEEP_TIME,
        RTC_WAKEUPCLOCK_CK_SPRE_16BITS,
        0
    );

    /* Enter STOP2 */
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);
}

void PowerUp(void)
{
    /* Restore system clock */
    SystemClock_Config();

    /* Resume SysTick */
    HAL_ResumeTick();

    /* Disable the RTC wake-up timer */
    HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);

    /* Reinitialize SPI */
    MX_SPI1_Init();

    NAU7802_PowerUp();

    /* Turn on LED to indicate we're awake */
    //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
}

void PowerOnDisplay(void)
{
	HAL_GPIO_WritePin(TFT_PWR_GPIO_Port,
					  TFT_PWR_Pin,
					  GPIO_PIN_SET);

	ST7735_Init();
	ST7735_Fill(ST7735_BLACK);
}

void PowerOffDisplay(void)
{
	ST7735_Fill(ST7735_BLACK);
	HAL_GPIO_WritePin(TFT_PWR_GPIO_Port,
					  TFT_PWR_Pin,
					  GPIO_PIN_RESET);
}

