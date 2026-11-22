/*
 * menu.c
 *
 *  Created on: Nov 5, 2026
 *      Author: mfdal
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "main.h"

#include "menu.h"
#include "st7735.h"
#include "gfx.h"
#include "fram_queue.h"
#include "rtc.h"
#include "nau7802.h"
#include "rfm95.h"
#include "power.h"
#include "hive_packet.h"

#define MENU_DRAW_INTERVAL  100 // ms
#define MENU_TIMEOUT        20000 // ms
#define BUTTON_DEBOUNCE_MS  30
#define BUTTON_LONG_MS      350
#define LIVE_TX_PERIOD      2000

#define COLOR_NORMAL_BG   ST7735_CHARCOAL
#define COLOR_NORMAL_FG   ST7735_OFFWHT

#define COLOR_SELECT_BG   ST7735_CHARCOAL
#define COLOR_SELECT_FG   ST7735_MAGENTA

static int8_t currentHighlighted = 0;
static int8_t currentSelected = 0;
static MenuScreen currentScreen = SCREEN_HOME;
static MenuScreen oldScreen = SCREEN_STATUS;

extern HivePacket packet;
extern CRC_HandleTypeDef hcrc;

uint8_t menu_tx_success = 0;
uint8_t menu_node_id;
uint16_t menu_sleep_time;
uint32_t last_screen_update = 0;

bool exit_menu = false;

ButtonEvent ReadButton(void) {

	static int lastReading = 0;
	static int buttonState = 0;

	static uint32_t lastDebounceTime = 0;
	static uint32_t pressStartTime = 0;

	static bool long_sent = false;

	uint32_t now = HAL_GetTick();

	// Read active-low button
	int reading = (HAL_GPIO_ReadPin(DEBUG_REQUEST_GPIO_Port,
	DEBUG_REQUEST_Pin) == GPIO_PIN_RESET);

	// Detect raw input changes
	if (reading != lastReading) {
		lastDebounceTime = now;
		lastReading = reading;
	}

	// Accept stable input
	if ((now - lastDebounceTime) >= BUTTON_DEBOUNCE_MS) {

		if (reading != buttonState) {

			buttonState = reading;

			// Button pressed
			if (buttonState) {
				pressStartTime = now;
				long_sent = false;
			}

			// Button released
			else {
				if (!long_sent) {
					return BUTTON_SHORT;
				}
			}
		}
	}

	// Long press detection
	if (buttonState && !long_sent) {

		if ((now - pressStartTime) >= BUTTON_LONG_MS) {
			long_sent = true;
			return BUTTON_LONG;
		}
	}

	return BUTTON_NONE;
}

static void uint32_to_bin(uint32_t val, char *buf, size_t buf_size) {
	if (buf_size < 33) {
		if (buf_size > 0)
			buf[0] = '\0';
		return;
	}

	for (int i = 31; i >= 0; i--) {
		buf[31 - i] = ((val >> i) & 1U) ? '1' : '0';
	}
	buf[32] = '\0';
}

static void DrawMenu(void) {
	if (currentScreen != oldScreen) {
		ST7735_Fill(ST7735_CHARCOAL);
		switch (currentScreen) {
		case SCREEN_STATUS:
			currentSelected = 1;
			break;
		case SCREEN_SETTINGS:
			currentSelected = 2;
		default:
			break;
		}
	}

	switch (currentScreen) {
	case SCREEN_HOME:
		for (uint8_t i = 0; i < MENU_COUNT; i++) {

			uint16_t bg;
			uint16_t fg;

			if (i == currentHighlighted) {
				bg = COLOR_SELECT_BG;
				fg = COLOR_SELECT_FG;
			} else {
				bg = COLOR_NORMAL_BG;
				fg = COLOR_NORMAL_FG;
			}

			// Draw item label
			GFX_SetTextColor(fg);
			GFX_SetTextBackground(bg);
			GFX_SetCursor(5, 5 + (i * 15));

			GFX_Print(menuItems[i]);
		}
		break;

	case SCREEN_STATUS:
		char date_buffer[12];
		char time_buffer[12];
		char backlog_buffer[24];
		RTC_GetDateString(date_buffer, sizeof(date_buffer));
		RTC_GetTimeString(time_buffer, sizeof(time_buffer));

		uint8_t rtc_valid = RTC_IsValid();

		uint16_t backlog = FRAM_GetCount();
		snprintf(backlog_buffer, sizeof(backlog_buffer),
				"Backlog: %d packets     ", backlog);

		GFX_SetTextColor(ST7735_OFFWHT);
		GFX_SetTextBackground(ST7735_CHARCOAL);
		GFX_SetCursor(5, 5);
		GFX_Print("RTC Date: ");
		GFX_Print(date_buffer);
		GFX_SetCursor(5, 17);
		GFX_Print("RTC Time: ");
		GFX_Print(time_buffer);

		if (rtc_valid) {
			GFX_SetCursor(5, 29);
			GFX_Print("RTC Valid");
		} else {
			GFX_SetTextColor(ST7735_RED);
			GFX_SetCursor(5, 29);
			GFX_Print("RTC Invalid");
		}

		if (backlog) {
			GFX_SetTextColor(ST7735_RED);
		} else {
			GFX_SetTextColor(ST7735_OFFWHT);
		}

		GFX_SetCursor(5, 41);
		GFX_Print(backlog_buffer);

		GFX_SetTextColor(currentSelected == 0 ? ST7735_MAGENTA : ST7735_OFFWHT);
		GFX_SetCursor(5, 53);
		GFX_Print("Clear Backlog");
		GFX_SetTextColor(currentSelected == 1 ? ST7735_MAGENTA : ST7735_OFFWHT);
		GFX_SetCursor(5, 65);
		GFX_Print("BACK");

		break;

	case SCREEN_SENSORS:
		int32_t weight;
		char weight_buffer[17];
		char bin_str[33];

		NAU7802_ReadADC(&weight);
		uint32_to_bin(weight, bin_str, sizeof(bin_str));
		snprintf(weight_buffer, sizeof(weight_buffer), "%ld              ",
				(long) weight);

		GFX_SetTextColor(ST7735_OFFWHT);
		GFX_SetTextBackground(ST7735_CHARCOAL);
		GFX_SetCursor(5, 5);
		GFX_Print("ADC: ");
		GFX_Print(weight_buffer);
		GFX_SetCursor(5, 17);
		GFX_Print(bin_str);

		GFX_SetTextColor(ST7735_RED);
		GFX_SetCursor(5, 29);
		GFX_Print("int. HDC3022: unavailable");

		GFX_SetCursor(5, 41);
		GFX_Print("ext. HDC3022: unavailable");

		GFX_SetCursor(5, 53);
		GFX_Print("BMP585: unavailable");

		GFX_SetTextColor(ST7735_MAGENTA);
		GFX_SetCursor(5, 65);
		GFX_Print("BACK");
		break;

	case SCREEN_TX:
		int16_t dbm;
		float snr;
		char dbm_buffer[36];
		char snr_buffer[36];

		GFX_SetTextColor(ST7735_OFFWHT);
		GFX_SetTextBackground(ST7735_CHARCOAL);
		GFX_SetCursor(5, 5);
		GFX_Print("Running live TX...");

		if (HAL_GetTick() - last_screen_update >= LIVE_TX_PERIOD) {
			last_screen_update = HAL_GetTick();
			packet.version = 4;
			packet.node_id = 0;
			packet.sequence++;

			packet.flags = 0;
			packet.flags |= FLAG_TEST_PACKET;

			packet.timestamp = (uint32_t) GetTimestamp();

			packet.weight = 0;
			packet.temperature = 0;
			packet.humidity = 0;

			packet.external_temperature = 0;
			packet.external_humidity = 0;

			packet.baro_pressure = 0;

			packet.rain_tips = 0;

			packet.crc = (uint16_t) HAL_CRC_Calculate(&hcrc,
					(uint32_t*) &packet, sizeof(packet) - sizeof(packet.crc));

			menu_tx_success = TransmitPacket(&packet);

			if (!menu_tx_success) {
				GFX_SetTextColor(ST7735_RED);
				GFX_SetCursor(5, 17);
				GFX_Print("Packet TX failed.    ");
			} else {
				lora_packet_rssi(&dbm);
				lora_packet_snr(&snr);
				snprintf(dbm_buffer, sizeof(dbm_buffer), "RSSI: %d dBm", dbm);

				int32_t snr_hundredths = (int32_t) (snr * 100.0f);
				uint32_t magnitude =
						(snr_hundredths < 0) ?
								(uint32_t) (-(int64_t) snr_hundredths) :
								(uint32_t) snr_hundredths;

				snprintf(snr_buffer, sizeof(snr_buffer), "SNR: %s%lu.%02lu dB",
						snr_hundredths < 0 ? "-" : "",
						(unsigned long) (magnitude / 100),
						(unsigned long) (magnitude % 100));

				GFX_SetTextColor(ST7735_GREEN);
				GFX_SetCursor(5, 17);
				GFX_Print("Packet TX succeeded.");

				GFX_SetTextColor(ST7735_OFFWHT);
				GFX_SetCursor(5, 29);
				GFX_Print(dbm_buffer);

				GFX_SetCursor(5, 41);
				GFX_Print(snr_buffer);
			}

			GFX_SetTextColor(ST7735_MAGENTA);
			GFX_SetCursor(5, 65);
			GFX_Print("BACK");
		}
		break;

	case SCREEN_SETTINGS:
		FRAM_GetNodeID(&menu_node_id);
		char node_id_buffer[14];
		snprintf(node_id_buffer, sizeof(node_id_buffer), "Node ID: %d ",
				menu_node_id);

		FRAM_GetSleepTime(&menu_sleep_time);
		char sleep_time_buffer[30];
		uint8_t sleep_time_mins = menu_sleep_time / 60;

		if (menu_sleep_time < 60) {
			snprintf(sleep_time_buffer, sizeof(sleep_time_buffer),
					"Sleep Time: %d secs     ", menu_sleep_time);
		} else if (sleep_time_mins == 1) {
			snprintf(sleep_time_buffer, sizeof(sleep_time_buffer),
					"Sleep Time: %d min     ", menu_sleep_time / 60);
		} else {
			snprintf(sleep_time_buffer, sizeof(sleep_time_buffer),
					"Sleep Time: %d mins     ", menu_sleep_time / 60);
		}

		GFX_SetTextBackground(ST7735_CHARCOAL);
		GFX_SetTextColor(currentSelected == 0 ? ST7735_MAGENTA : ST7735_OFFWHT);
		GFX_SetCursor(5, 5);
		GFX_Print(node_id_buffer);

		GFX_SetCursor(5, 17);
		GFX_SetTextColor(currentSelected == 1 ? ST7735_MAGENTA : ST7735_OFFWHT);
		GFX_Print(sleep_time_buffer);

		GFX_SetTextColor(currentSelected == 2 ? ST7735_MAGENTA : ST7735_OFFWHT);
		GFX_SetCursor(5, 65);
		GFX_Print("BACK");
		break;

	case SCREEN_EXIT:
		ST7735_Fill(ST7735_BLACK);
		exit_menu = true;
		break;

	}
}

static void HandleShortPress(void) {
	switch (currentScreen) {
	case SCREEN_HOME:
		if (currentHighlighted == 4) {
			exit_menu = true;
		}

		currentScreen = currentHighlighted + 1;
		break;
	case SCREEN_STATUS:
		if (currentSelected == 1) {
			currentScreen = SCREEN_HOME;
		} else if (currentSelected == 0) {
			FRAM_QueueInit(); // clear the queue
		}
		break;

	case SCREEN_SETTINGS:
		if (currentSelected == 0) {
			FRAM_GetNodeID(&menu_node_id);
			menu_node_id++;
			if (menu_node_id > 10) {
				menu_node_id = 0;
			}
			FRAM_SetNodeID(menu_node_id);
		} else if (currentSelected == 1) {
			FRAM_GetSleepTime(&menu_sleep_time);
			if (menu_sleep_time < 60) {
				menu_sleep_time += 5;
			} else if (60 <= menu_sleep_time && menu_sleep_time < 600) {
				menu_sleep_time += 60;
			} else {
				menu_sleep_time = 5;
			}
			FRAM_SetSleepTime(menu_sleep_time);
		} else if (currentSelected == 2) {
			currentScreen = SCREEN_HOME;
		}
		break;
	default:
		currentScreen = SCREEN_HOME;
		break;

	}
}

static void HandleLongPress(void) {
	switch (currentScreen) {
	case SCREEN_HOME:
		currentSelected = 0;
		currentHighlighted++;
		if (currentHighlighted >= MENU_COUNT) {
			currentHighlighted = 0;
		}
		break;
	case SCREEN_STATUS:
		currentSelected++;
		if (currentSelected >= 2) {
			currentSelected = 0;
		}
		break;

	case SCREEN_SETTINGS:
		currentSelected++;
		if (currentSelected > 2) {
			currentSelected = 0;
		}
		break;
	default:
		break;
	}
}

void RunMenu(void) {
	ButtonEvent menu_action;

	currentHighlighted = 0;
	currentSelected = 0;
	currentScreen = SCREEN_HOME;
	oldScreen = SCREEN_STATUS;

	exit_menu = false;
	uint32_t last_draw = HAL_GetTick();
	uint32_t last_action = HAL_GetTick();

	while (!exit_menu) {

		menu_action = ReadButton();

		switch (menu_action) {

		case BUTTON_LONG:
			last_action = HAL_GetTick();
			HandleLongPress();
			break;

		case BUTTON_SHORT:
			last_action = HAL_GetTick();
			HandleShortPress();
			break;

		case BUTTON_NONE:
			break;
		}

		if (HAL_GetTick() - last_action >= MENU_TIMEOUT) {
			switch (currentScreen) {
			case SCREEN_TX:
				break;
			default:
				exit_menu = 1;
			}
		}

		if (HAL_GetTick() - last_draw >= MENU_DRAW_INTERVAL) {
			DrawMenu();
			last_draw = HAL_GetTick();
			oldScreen = currentScreen; // we redrew
		}

	}
}
