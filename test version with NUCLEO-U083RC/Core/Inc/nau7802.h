/*
 * nau7802.h
 *
 *  Created on: Sep 15, 2026
 *      Author: mfdal
 */

#ifndef INC_NAU7802_H_
#define INC_NAU7802_H_

uint8_t NAU7802_Init(void);

uint8_t NAU7802_Reset(void);

uint8_t NAU7802_PowerUp(void);
uint8_t NAU7802_PowerDown(void);

HAL_StatusTypeDef NAU7802_ReadADC(int32_t *value);

#endif /* INC_NAU7802_H_ */
