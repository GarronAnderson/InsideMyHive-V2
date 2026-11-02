/*
 * hive_packet.h
 *
 *  Created on: Sep 25, 2026
 *      Author: mfdal
 */

#ifndef INC_HIVE_PACKET_H_
#define INC_HIVE_PACKET_H_

#include <stdint.h>
#include <time.h>

typedef struct __attribute__((packed))
{
    uint8_t  version;
    uint8_t  node_id;
    uint16_t sequence;

    uint32_t timestamp;

    uint8_t  backlog;

    int32_t  weight;
    int16_t  temperature;
    uint16_t humidity;

    int16_t  external_temperature;
    uint16_t external_humidity;

    uint16_t crc;
} HivePacket;

typedef struct __attribute__((packed))
{
    uint16_t sequence;
    uint16_t crc;
} HiveAck;

void Set_RTC_Default_Time(void);

time_t GetTimestamp(void);

HivePacket ReadSensors(void);

uint8_t TransmitPacket(HivePacket *packet);

#endif /* INC_HIVE_PACKET_H_ */
