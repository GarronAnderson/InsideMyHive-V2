/*
 * fram.h
 *
 *  Created on: Sep 24, 2026
 *      Author: mfdal
 */

#ifndef INC_FRAM_H_
#define INC_FRAM_H_

#include "main.h"

#define FRAM_SIZE            131072UL

HAL_StatusTypeDef FRAM_Init(void);

HAL_StatusTypeDef FRAM_Read(uint32_t address, uint8_t *data, uint16_t length);

HAL_StatusTypeDef FRAM_Write(uint32_t address, const uint8_t *data,
		uint16_t length);

HAL_StatusTypeDef FRAM_ReadStatus(uint8_t *status);

HAL_StatusTypeDef FRAM_WriteEnable(void);

HAL_StatusTypeDef FRAM_Sleep(void);

HAL_StatusTypeDef FRAM_Wake(void);

#endif /* INC_FRAM_H_ */
