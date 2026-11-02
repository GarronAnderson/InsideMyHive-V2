/*
 * fram_queue.h
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#ifndef INC_FRAM_QUEUE_H_
#define INC_FRAM_QUEUE_H_

#include "hive_packet.h"

void FRAM_QueueInit(void);
HAL_StatusTypeDef FRAM_Push(const HivePacket *packet);
HAL_StatusTypeDef FRAM_Peek(HivePacket *packet);
HAL_StatusTypeDef FRAM_Pop(void);
uint16_t FRAM_GetCount(void);

#endif /* INC_FRAM_QUEUE_H_ */
