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
#include <string.h>

#define MAX_RETRIES 3

extern RTC_HandleTypeDef hrtc;

RTC_TimeTypeDef currentTime;
RTC_DateTypeDef currentDate;

time_t timestamp;
struct tm currTime;

void Set_RTC_Default_Time(void) {
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    // Setting to April 15, 2026 at 14:30:00 UTC
    sTime.Hours = 14;
    sTime.Minutes = 30;
    sTime.Seconds = 00;
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    sDate.Year = 26; // 2026
    sDate.Month = RTC_MONTH_SEPTEMBER;
    sDate.Date = 25;
    sDate.WeekDay = RTC_WEEKDAY_FRIDAY;
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

time_t GetTimestamp(void) {
	HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);

	currTime.tm_year = currentDate.Year + 100;
	currTime.tm_mon  = currentDate.Month - 1;
	currTime.tm_mday = currentDate.Date;

	currTime.tm_hour = currentTime.Hours;
	currTime.tm_min  = currentTime.Minutes;
	currTime.tm_sec  = currentTime.Seconds;

	currTime.tm_isdst = 0;

	timestamp = mktime(&currTime);

	return timestamp;
}

uint8_t TransmitPacket(HivePacket *packet)
{
	// Try to transmit a packet MAX_RETRIES times.
	// Returns 1 for success, 0 for fail.

	uint8_t retries = 0;
	while(retries < MAX_RETRIES)
	{
		HAL_StatusTypeDef txresult =
			lora_TX((uint8_t *)packet, sizeof(*packet), 1000);

		if (txresult != HAL_OK)
		{
			retries++;
			continue;
		}

	  uint8_t ack_buffer[8];
	  uint8_t ack_length = 0;

	  HAL_StatusTypeDef rxresult = lora_RX(ack_buffer, &ack_length, sizeof(ack_buffer), 1000);

	  if (rxresult == HAL_OK)
	  {
		  if (ack_length == 8)
		  {
			  HiveAck ack;

			  memcpy(&ack, &ack_buffer[4], sizeof(ack));

			  if (ack.sequence == packet->sequence &&
				  ack.crc == packet->crc)
			  {
				  return 1;
			  }
		  }
	  } else {
		  // we timed out (other side didn't respond)
		  retries++;
	  }

	}
	return 0;
}
