/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TFT_PWR_Pin GPIO_PIN_4
#define TFT_PWR_GPIO_Port GPIOA
#define TFT_CS_Pin GPIO_PIN_0
#define TFT_CS_GPIO_Port GPIOB
#define TFT_RST_Pin GPIO_PIN_1
#define TFT_RST_GPIO_Port GPIOB
#define TFT_DC_Pin GPIO_PIN_2
#define TFT_DC_GPIO_Port GPIOB
#define RFM_RST_Pin GPIO_PIN_10
#define RFM_RST_GPIO_Port GPIOB
#define RFM_CS_Pin GPIO_PIN_11
#define RFM_CS_GPIO_Port GPIOB
#define RFM_IRQ_Pin GPIO_PIN_8
#define RFM_IRQ_GPIO_Port GPIOA
#define RFM_IRQ_EXTI_IRQn EXTI4_15_IRQn
#define DEBUG_REQUEST_Pin GPIO_PIN_10
#define DEBUG_REQUEST_GPIO_Port GPIOA
#define DEBUG_REQUEST_EXTI_IRQn EXTI4_15_IRQn
#define FRAM_CS_Pin GPIO_PIN_15
#define FRAM_CS_GPIO_Port GPIOA
#define NAU_DRDY_Pin GPIO_PIN_6
#define NAU_DRDY_GPIO_Port GPIOB
#define NAU_DRDY_EXTI_IRQn EXTI4_15_IRQn
#define RAIN_EXTI_Pin GPIO_PIN_7
#define RAIN_EXTI_GPIO_Port GPIOB
#define RAIN_EXTI_EXTI_IRQn EXTI4_15_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
