/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "fram.h"
#include "power.h"

#include "driver_htu21d.h"
#include "driver_htu21d_interface.h"

#include "st7735.h"
#include "gfx.h"

#include "nau7802.h"

#include "rfm95.h"

#include "fram_queue.h"
#include "hive_packet.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

COM_InitTypeDef BspCOMInit;

CRC_HandleTypeDef hcrc;

I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;

/* USER CODE BEGIN PV */

htu21d_handle_t htu21d;

HivePacket packet;

volatile uint8_t debug_requested = 0;

uint8_t tx_success = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
void MX_SPI1_Init(void);
static void MX_RTC_Init(void);
static void MX_CRC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_RTC_Init();
  MX_CRC_Init();
  /* USER CODE BEGIN 2 */

  Set_RTC_Default_Time();

  FRAM_Init();
  FRAM_QueueInit();

  	uint8_t res;
	uint16_t temperature_raw;
	uint16_t humidity_raw;
	float temperature;
	float humidity;

	DRIVER_HTU21D_LINK_INIT(&htu21d, htu21d_handle_t);
	DRIVER_HTU21D_LINK_IIC_INIT(&htu21d, htu21d_interface_iic_init);
	DRIVER_HTU21D_LINK_IIC_DEINIT(&htu21d, htu21d_interface_iic_deinit);
	DRIVER_HTU21D_LINK_IIC_WRITE_CMD(&htu21d, htu21d_interface_iic_write_cmd);
	DRIVER_HTU21D_LINK_IIC_READ_CMD(&htu21d, htu21d_interface_iic_read_cmd);
	DRIVER_HTU21D_LINK_IIC_WRITE(&htu21d, htu21d_interface_iic_write);
	DRIVER_HTU21D_LINK_IIC_READ(&htu21d, htu21d_interface_iic_read);
	DRIVER_HTU21D_LINK_IIC_READ_WITH_SCL(&htu21d, htu21d_interface_iic_read_with_scl);
	DRIVER_HTU21D_LINK_DELAY_MS(&htu21d, htu21d_interface_delay_ms);
	DRIVER_HTU21D_LINK_DEBUG_PRINT(&htu21d, htu21d_interface_debug_print);

	res = htu21d_init(&htu21d);

	if (res != 0)
	{
		/* Sensor initialization failed */
		Error_Handler();
	}

	PowerOnDisplay();
	ST7735_Init();
	ST7735_Fill(ST7735_BLACK);

	GFX_Init();

	res = NAU7802_Init();

	if (res != 0)
	{
		Error_Handler();
	}

	int32_t weight;

	SX1276_HandleTypedef rfm95 = {
		.hspi = &hspi1,
		.nss_port = RFM_CS_GPIO_Port,
		.nss_pin = RFM_CS_Pin,
		.rst_port = RFM_RST_GPIO_Port,
		.rst_pin = RFM_RST_Pin
	};

	LORA_CONFIG_TYPEDEF lora_config = {
		.frequency = 915000000UL,
		.spreading_Factor = 7,
		.bandwidth = 125000,
		.coding_Rate = 5,
		.tx_Power = 17,
		.use_Pa_Boost = 1,
		.preamble_Length = 8,
		.sync_Word = 0x12,
		.enable_CRC = 1,
		.implicit_header = 0
	};

	lora_init(&rfm95, &lora_config);

	uint8_t retries = 0;

  /* USER CODE END 2 */

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Initialize COM1 port (115200, 8 bits (7-bit data + 1 stop bit), no parity */
  BspCOMInit.BaudRate   = 115200;
  BspCOMInit.WordLength = COM_WORDLENGTH_8B;
  BspCOMInit.StopBits   = COM_STOPBITS_1;
  BspCOMInit.Parity     = COM_PARITY_NONE;
  BspCOMInit.HwFlowCtl  = COM_HWCONTROL_NONE;
  if (BSP_COM_Init(COM1, &BspCOMInit) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  // Just woke up
  PowerUp();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

  if(debug_requested){
	  PowerOnDisplay();

	  GFX_SetCursor(2, 2);
	  GFX_SetTextColor(ST7735_GREEN);
	  GFX_Print("DEBUG");
	  GFX_SetTextColor(ST7735_WHITE);

	  while(debug_requested){
		  res = htu21d_read_temperature_humidity(
			  &htu21d,
			  &temperature_raw,
			  &temperature,
			  &humidity_raw,
			  &humidity
		  );

		  NAU7802_ReadADC(&weight);

		  char buffer[32];
		  temperature = (temperature * 9 / 5) + 32;
		  sprintf(buffer, "Hive Temp: %.2f F", temperature);
		  GFX_SetCursor(2, 12);
		  GFX_Print(buffer);

		  sprintf(buffer, "Hive Hum.: %.2f %%", humidity);
		  GFX_SetCursor(2, 24);
		  GFX_Print(buffer);

		  sprintf(buffer, "Weight: %ld", weight);
		  GFX_SetCursor(2, 36);
		  GFX_Print(buffer);

		  HAL_Delay(200);
	  }
  }

  // Powered On

  res = htu21d_read_temperature_humidity(
	  &htu21d,
	  &temperature_raw,
	  &temperature,
	  &humidity_raw,
	  &humidity
  );

  NAU7802_ReadADC(&weight);

  packet.version = 1;
  packet.node_id = 1;
  packet.sequence++;

  packet.timestamp = (uint32_t)GetTimestamp();

  packet.weight = weight;
  packet.temperature = temperature*100;
  packet.humidity = humidity*100;

  packet.external_temperature = 0;
  packet.external_humidity = 0;

  packet.crc = (uint16_t)HAL_CRC_Calculate(
      &hcrc,
      (uint32_t *)&packet,
      sizeof(packet) - sizeof(packet.crc)
  );

  tx_success = TransmitPacket(&packet);

  if(!tx_success)
  {
	  // failed after a packet TX
	  packet.backlog++;
	  packet.crc = (uint16_t)HAL_CRC_Calculate(
	        &hcrc,
	        (uint32_t *)&packet,
	        sizeof(packet) - sizeof(packet.crc)
	    );
	  HAL_StatusTypeDef push_status = FRAM_Push(&packet);
	  if(push_status != HAL_OK){
		  Error_Handler();
	  }
  }

  if(tx_success)
  {
	// okay, retry
	while(FRAM_GetCount()){
		if(FRAM_Peek(&packet) == HAL_OK)
		{
			tx_success = TransmitPacket(&packet);
			if(!tx_success){
				break;
			}
			FRAM_Pop();
		}
	}
	if(FRAM_GetCount() == 0){
		// no backlog
		packet.backlog = 0;
		packet.crc = (uint16_t)HAL_CRC_Calculate(
			  &hcrc,
			  (uint32_t *)&packet,
			  sizeof(packet) - sizeof(packet.crc)
		);
	}
  }

  // Power Down
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  PowerOffDisplay();
  EnterSleep();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
  hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
  hcrc.Init.GeneratingPolynomial = 4129;
  hcrc.Init.CRCLength = CRC_POLYLENGTH_16B;
  hcrc.Init.InitValue = 0xFFFF;
  hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
  hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00303D5B;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Enable the WakeUp
  */
  if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0, RTC_WAKEUPCLOCK_RTCCLK_DIV16, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, RFM_CS_Pin|RFM_RST_Pin|FERAM_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, TFT_CS_Pin|TFT_DC_Pin|TFT_RST_Pin|TFT_PWR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : RFM_CS_Pin RFM_RST_Pin FERAM_CS_Pin */
  GPIO_InitStruct.Pin = RFM_CS_Pin|RFM_RST_Pin|FERAM_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : RFM_IRQ_Pin */
  GPIO_InitStruct.Pin = RFM_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(RFM_IRQ_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : TFT_CS_Pin TFT_DC_Pin TFT_RST_Pin TFT_PWR_Pin */
  GPIO_InitStruct.Pin = TFT_CS_Pin|TFT_DC_Pin|TFT_RST_Pin|TFT_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : DEBUG_REQUEST_Pin */
  GPIO_InitStruct.Pin = DEBUG_REQUEST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DEBUG_REQUEST_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DEBUG_REQUEST_Pin)
    {
        debug_requested = 1;
    }
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == DEBUG_REQUEST_Pin)
    {
        debug_requested = 0;
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
