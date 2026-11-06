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

#define BUTTON_DEBOUNCE_MS  30
#define BUTTON_LONG_MS      750

#define COLOR_NORMAL_BG   ST7735_BLACK
#define COLOR_NORMAL_FG   ST7735_WHITE

#define COLOR_SELECT_BG   ST7735_BLUE
#define COLOR_SELECT_FG   ST7735_WHITE

static int8_t currentHighlighted = 0;
static MenuScreen currentScreen = SCREEN_HOME;
static MenuScreen oldScreen = SCREEN_STATUS;

extern HivePacket packet;
extern CRC_HandleTypeDef hcrc;

uint8_t menu_tx_success = 0;

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

static void DrawMenu(void) {
	if (currentScreen != oldScreen) {
		ST7735_Fill(ST7735_BLACK);
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
		snprintf(backlog_buffer, sizeof(backlog_buffer), "Backlog: %d packets",
				backlog);

		GFX_SetTextColor(ST7735_WHITE);
		GFX_SetTextBackground(ST7735_BLACK);
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
			GFX_SetTextBackground(ST7735_RED);
			GFX_SetCursor(5, 29);
			GFX_Print("RTC Invalid");
		}

		GFX_SetTextBackground(ST7735_BLACK);
		GFX_SetCursor(5, 41);
		GFX_Print(backlog_buffer);

		GFX_SetTextBackground(ST7735_BLUE);
		GFX_SetCursor(5, 70);
		GFX_Print("BACK");
		break;
	case SCREEN_SENSORS:
		int32_t weight;
		char weight_buffer[17];

		NAU7802_ReadADC(&weight);
		snprintf(weight_buffer, sizeof(weight_buffer), "%ld              ",
				(long) weight);

		GFX_SetTextColor(ST7735_WHITE);
		GFX_SetTextBackground(ST7735_BLACK);
		GFX_SetCursor(5, 5);
		GFX_Print("ADC: ");
		GFX_Print(weight_buffer);

		GFX_SetCursor(5, 17);
		GFX_Print("int. HDC3022: unavailable");

		GFX_SetCursor(5, 29);
		GFX_Print("ext. HDC3022: unavailable");

		GFX_SetCursor(5, 41);
		GFX_Print("BMP585: unavailable");

		GFX_SetTextBackground(ST7735_BLUE);
		GFX_SetCursor(5, 70);
		GFX_Print("BACK");
		break;

	case SCREEN_TX:
		int16_t dbm;
		float snr;
		char dbm_buffer[36];
		char snr_buffer[36];

		GFX_SetTextColor(ST7735_WHITE);
		GFX_SetTextBackground(ST7735_BLACK);
		GFX_SetCursor(5, 5);
		GFX_Print("Running live TX...");

		packet.version = 2;
		packet.node_id = 0;
		packet.sequence++;

		packet.flags = 0;

		packet.timestamp = (uint32_t) GetTimestamp();

		packet.weight = 0;
		packet.temperature = 0;
		packet.humidity = 0;

		packet.external_temperature = 0;
		packet.external_humidity = 0;

		packet.crc = (uint16_t) HAL_CRC_Calculate(&hcrc, (uint32_t*) &packet,
				sizeof(packet) - sizeof(packet.crc));

		menu_tx_success = TransmitPacket(&packet);

		if (!menu_tx_success) {
			GFX_SetTextBackground(ST7735_RED);
			GFX_SetCursor(5, 17);
			GFX_Print("Packet TX failed.");
		} else {
			lora_packet_rssi(&dbm);
			lora_packet_snr(&snr);
			snprintf(dbm_buffer, sizeof(dbm_buffer), "RSSI: %d dBm", dbm);
			snprintf(snr_buffer, sizeof(snr_buffer), "SNR: %.2f dB", snr);

			GFX_SetTextBackground(ST7735_GREEN);
			GFX_SetCursor(5, 17);
			GFX_Print("Packet TX succeeded.");

			GFX_SetTextBackground(ST7735_BLACK);
			GFX_SetCursor(5, 29);
			GFX_Print(dbm_buffer);

			GFX_SetCursor(5, 41);
			GFX_Print(snr_buffer);

			GFX_SetTextBackground(ST7735_BLUE);
			GFX_SetCursor(5, 70);
			GFX_Print("BACK");
		}
		HAL_Delay(2000);
		break;
	}
}

static void NextMenuItem(void) {
	currentHighlighted++;

	if (currentHighlighted >= MENU_COUNT) {
		currentHighlighted = 0;
	}
}

static void SelectItem(void) {
	switch (currentScreen) {
	case SCREEN_HOME:
		currentScreen = currentHighlighted + 1;
		break;
	default:
		currentScreen = SCREEN_HOME;
	}
}

void RunMenu(void) {
	ButtonEvent menu_action;
	bool exit = false;
	bool main_menu = true;

	while (!exit) {

		menu_action = ReadButton();

		switch (menu_action) {

		case BUTTON_LONG:
			SelectItem();

		case BUTTON_SHORT:
			NextMenuItem();

		case BUTTON_NONE:
			break;
		}

		if (main_menu) {
			DrawMenu();
		}
		oldScreen = currentScreen;
	}
}
