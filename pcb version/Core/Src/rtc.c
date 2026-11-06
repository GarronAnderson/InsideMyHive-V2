/*
 * rtc.c
 *
 *  Created on: Nov 6, 2026
 *      Author: mfdal
 */

#include "main.h"
#include <time.h>
#include <string.h>
#include <stdio.h>
#include "rtc.h"

extern RTC_HandleTypeDef hrtc;

RTC_TimeTypeDef currentTime;
RTC_DateTypeDef currentDate;

time_t timestamp;
struct tm currTime;

time_t GetTimestamp(void) {
	HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN);

	currTime.tm_year = currentDate.Year + 100;
	currTime.tm_mon = currentDate.Month - 1;
	currTime.tm_mday = currentDate.Date;

	currTime.tm_hour = currentTime.Hours;
	currTime.tm_min = currentTime.Minutes;
	currTime.tm_sec = currentTime.Seconds;

	currTime.tm_isdst = 0;

	timestamp = mktime(&currTime);

	return timestamp;
}

uint8_t RTC_IsValid(void) {
	return (uint32_t) GetTimestamp() >= RTC_VALID_MIN_UNIX;
}

void RTC_SetFromUnix(uint32_t unix_time) {
	time_t t = unix_time;
	struct tm tm;
	gmtime_r(&t, &tm);

	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };

	sTime.Hours = tm.tm_hour;
	sTime.Minutes = tm.tm_min;
	sTime.Seconds = tm.tm_sec;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	sDate.Year = tm.tm_year - 100;                       // 2000-2099
	sDate.Month = tm.tm_mon + 1;
	sDate.Date = tm.tm_mday;
	sDate.WeekDay = (tm.tm_wday == 0) ? 7 : tm.tm_wday;     // HAL: Mon=1..Sun=7

	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void RTC_GetDateString(char *time_str, size_t max_len) {
	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	snprintf(time_str, max_len, "20%02d-%02d-%02d", sDate.Year, sDate.Month,
			sDate.Date);
}

void RTC_GetTimeString(char *time_str, size_t max_len) {
	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };

	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	snprintf(time_str, max_len, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes,
			sTime.Seconds);
}
