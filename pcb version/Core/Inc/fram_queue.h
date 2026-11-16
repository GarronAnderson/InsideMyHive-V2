/*
 * fram_queue.h
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#ifndef INC_FRAM_QUEUE_H_
#define INC_FRAM_QUEUE_H_

#include "hive_packet.h"

HAL_StatusTypeDef FRAM_QueueInit(void);
HAL_StatusTypeDef FRAM_Push(const HivePacket *packet);
HAL_StatusTypeDef FRAM_Peek(HivePacket *packet);
HAL_StatusTypeDef FRAM_Pop(void);
uint16_t FRAM_GetCount(void);

HAL_StatusTypeDef FRAM_SetNodeID(uint8_t node_id);
HAL_StatusTypeDef FRAM_GetNodeID(uint8_t *node_id);

HAL_StatusTypeDef FRAM_SetSleepTime(uint16_t sleep_time);
HAL_StatusTypeDef FRAM_GetSleepTime(uint16_t *sleep_time);

#endif /* INC_FRAM_QUEUE_H_ */
