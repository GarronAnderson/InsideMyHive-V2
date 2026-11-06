/*
 * menu.h
 *
 *  Created on: Nov 5, 2026
 *      Author: mfdal
 */

#ifndef INC_MENU_H_
#define INC_MENU_H_

void RunMenu(void);

typedef enum {
	BUTTON_NONE, BUTTON_SHORT, BUTTON_LONG
} ButtonEvent;

typedef enum {
	SCREEN_HOME, SCREEN_STATUS, SCREEN_SENSORS, SCREEN_TX, SCREEN_SETTINGS
} MenuScreen;

static const char *menuItems[] =
		{ "STATUS", "SENSORS", "TX", "SETTINGS", "EXIT" };

#define MENU_COUNT (sizeof(menuItems) / sizeof(menuItems[0]))

#endif /* INC_MENU_H_ */
