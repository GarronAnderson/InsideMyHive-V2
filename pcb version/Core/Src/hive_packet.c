/*
 * hive_packet.c
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#include "main.h"
#include "hive_packet.h"
#include "rfm95.h"
#include <time.h>
#include "rtc.h"
#include <string.h>
#include "fram_queue.h"

#define MAX_RETRIES 3

#define ACK_HDR_LEN           4
#define ACK_LEN               (ACK_HDR_LEN + sizeof(HiveAck))        // 9
#define ACK_LEN_WITH_TIME     (ACK_LEN + sizeof(uint32_t))           // 13
#define ACK_LEN_WITH_INTERVAL (ACK_LEN + sizeof(uint16_t))           // 11

uint8_t TransmitPacket(HivePacket *packet) {
	for (uint8_t retries = 0; retries < MAX_RETRIES; retries++) {
		if (lora_TX((uint8_t*) packet, sizeof(*packet), 1000) != HAL_OK)
			continue;

		uint8_t ack_buffer[16];
		uint8_t ack_length = 0;

		if (lora_RX(ack_buffer, &ack_length, sizeof(ack_buffer), 1000)
				!= HAL_OK)
			continue;

		if (ack_length != ACK_LEN && ack_length != ACK_LEN_WITH_TIME
				&& ack_length != ACK_LEN_WITH_INTERVAL)
			continue;

		HiveAck ack;
		memcpy(&ack, &ack_buffer[ACK_HDR_LEN], sizeof(ack));
		if (ack.sequence != packet->sequence || ack.crc != packet->crc)
			continue;

		if ((packet->flags & FLAG_TIME_REQUEST)
				&& ack_length == ACK_LEN_WITH_TIME
				&& (ack.flags & REPLY_FLAG_TIME_PACKET)) {
			uint32_t t;
			memcpy(&t, &ack_buffer[ACK_LEN], sizeof(t));
			if (t >= RTC_VALID_MIN_UNIX)
				RTC_SetFromUnix(t);
		}

		if (ack_length == ACK_LEN_WITH_INTERVAL
				&& (ack.flags & REPLY_FLAG_INTERVAL)) {
			uint16_t interval;
			memcpy(&interval, &ack_buffer[ACK_LEN], sizeof(interval));
			if (interval >= 5)
				FRAM_SetSleepTime(interval);
		}

		return 1;
	}
	return 0;
}
