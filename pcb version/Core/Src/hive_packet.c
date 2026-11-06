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

#define MAX_RETRIES 3

#define ACK_HDR_LEN         4
#define ACK_LEN             (ACK_HDR_LEN + sizeof(HiveAck))        // 8
#define ACK_LEN_WITH_TIME   (ACK_LEN + sizeof(uint32_t))           // 12

uint8_t TransmitPacket(HivePacket *packet) {
	for (uint8_t retries = 0; retries < MAX_RETRIES; retries++) {
		if (lora_TX((uint8_t*) packet, sizeof(*packet), 1000) != HAL_OK)
			continue;

		uint8_t ack_buffer[16];
		uint8_t ack_length = 0;

		if (lora_RX(ack_buffer, &ack_length, sizeof(ack_buffer), 1000)
				!= HAL_OK)
			continue;

		if (ack_length != ACK_LEN && ack_length != ACK_LEN_WITH_TIME)
			continue;

		HiveAck ack;
		memcpy(&ack, &ack_buffer[ACK_HDR_LEN], sizeof(ack));
		if (ack.sequence != packet->sequence || ack.crc != packet->crc)
			continue;

		if ((packet->flags & FLAG_TIME_REQUEST)
				&& ack_length == ACK_LEN_WITH_TIME) {
			uint32_t t;
			memcpy(&t, &ack_buffer[ACK_LEN], sizeof(t));
			if (t >= RTC_VALID_MIN_UNIX)
				RTC_SetFromUnix(t);
		}
		return 1;
	}
	return 0;
}
