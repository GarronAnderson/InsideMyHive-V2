/*
 * rtc.h
 *
 *  Created on: Nov 6, 2026
 *      Author: mfdal
 */

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include <stdint.h>
#include <time.h>

#define FLAG_TIME_REQUEST   (1u << 0)
#define RTC_VALID_MIN_UNIX  1767225600UL   // 2026-01-01 UTC; below this = RTC never set

uint8_t RTC_IsValid(void);
void RTC_SetFromUnix(uint32_t unix_time);
time_t GetTimestamp(void);
void RTC_GetTimeString(char *time_str, size_t max_len);
void RTC_GetDateString(char *time_str, size_t max_len);

#endif /* INC_RTC_H_ */
