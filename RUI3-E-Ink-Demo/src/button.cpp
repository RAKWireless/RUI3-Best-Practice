/**
 * @file button.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Button interrupt handler
 * @version 0.1
 * @date 2025-08-13
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "../inc/app.h"

uint8_t event = 0;

void handle_OTAA(void)
{
	detachInterrupt(LEFT_BUTTON);
	MYLOG("BT", "Button Left");

	event = 1;
	api.system.timer.start(RAK_TIMER_0, 100, &event);
}

void handle_ABP(void)
{
	detachInterrupt(RIGHT_BUTTON);
	MYLOG("BT", "Button Right");

	event = 2;
	api.system.timer.start(RAK_TIMER_0, 100, &event);
}

void handle_SEND(void)
{
	detachInterrupt(MIDDLE_BUTTON);
	MYLOG("BT", "Button Middle");
	if (api.lorawan.njs.get() == 0)
	{
		// rak14000_text(60, 20, "Network not joined", COLORED, 2);
		// refresh_rak14000();
		attachInterrupt(MIDDLE_BUTTON, handle_SEND, FALLING);\
		return;
	}
	event = 3;
	api.system.timer.start(RAK_TIMER_0, 100, &event);
}