/*
 * hive_packet.h
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#ifndef INC_HIVE_PACKET_H_
#define INC_HIVE_PACKET_H_

// flags send by the node to the inside unit
#define FLAG_TIME_REQUEST      (1u << 0)
#define FLAG_INT_HDC3022_ERROR (1u << 1)
#define FLAG_EXT_HDC3022_ERROR (1u << 2)
#define FLAG_BMP585_ERROR      (1u << 3)
#define FLAG_NAU7802_ERROR     (1u << 4)
#define FLAG_TEST_PACKET       (1u << 5)

// flag replies from the inside unit
#define REPLY_FLAG_TIME_PACKET (1u << 6)
#define REPLY_FLAG_INTERVAL    (1u << 7)

typedef struct __attribute__((packed)) {
	uint8_t version;
	uint8_t node_id;
	uint16_t sequence;

	uint32_t timestamp;

	uint8_t flags;

	uint16_t backlog;

	int32_t weight;
	int16_t temperature; // 0.01 deg C
	uint16_t humidity;  // 0.01 % RH

	int16_t external_temperature;
	uint16_t external_humidity;

	uint16_t baro_pressure; // 0.1 hPa

	uint16_t rain_tips;

	uint16_t crc;
} HivePacket;

typedef struct __attribute__((packed)) {
	uint16_t sequence;
	uint16_t crc;
	uint8_t flags;
} HiveAck;

uint8_t TransmitPacket(HivePacket *packet);

#endif /* INC_HIVE_PACKET_H_ */
