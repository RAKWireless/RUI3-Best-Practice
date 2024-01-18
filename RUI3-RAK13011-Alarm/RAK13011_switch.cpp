/**
 * @file RAK13011_switch.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RAK13011 Reed relay switch
 * @version 0.1
 * @date 2022-10-30
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include "ArrayQueue.h"

//******************************************************************//
// RAK13011 INT1_PIN
//******************************************************************//
// Slot A      WB_IO1
// Slot B      WB_IO2 ( not recommended, pin conflict with IO2)
// Slot C      WB_IO3
// Slot D      WB_IO5
// Slot E      WB_IO4
// Slot F      WB_IO6
//******************************************************************//

#define RAK13011_SLOT 'A'

/** Interrupt pin, depends on slot */
// #include "RAK13011_switch_S_A.h"

/** Interrupt pin, depends on slot */
// Slot A
#if RAK13011_SLOT == 'A'
#pragma message "Slot A"
#define SW_INT_PIN WB_IO1
// Slot B
#elif RAK13011_SLOT == 'B'
#pragma message "Slot B"
#define SW_INT_PIN WB_IO2
// Slot C
#elif RAK13011_SLOT == 'C'
#pragma message "Slot C"
#define SW_INT_PIN WB_IO3
// Slot D
#elif RAK13011_SLOT == 'D'
#pragma message "Slot D"
#define SW_INT_PIN WB_IO5
// Slot E
#elif RAK13011_SLOT == 'E'
#pragma message "Slot E"
#define SW_INT_PIN WB_IO4
// Slot F
#elif RAK13011_SLOT == 'F'
#pragma message "Slot F"
#define SW_INT_PIN WB_IO6
#endif

ArrayQueue Fifo;

volatile int switch_status = 0;

volatile bool handler_available = true;

void switch_int_handler(void)
{
	MYLOG("REED", "Interrupt, start bounce check");
	switch_status = digitalRead(SW_INT_PIN);
	api.system.timer.start(RAK_TIMER_2, 50, NULL);
}

void switch_bounce_check(void *)
{
	MYLOG("REED", "Bounce check");
	// Serial.println("Interrupt");
	int new_switch_status = digitalRead(SW_INT_PIN);
	if (new_switch_status != switch_status)
	{
		MYLOG("REED", "Bounce detected");
		return;
	}
	if (switch_status == LOW)
	{
		// digitalWrite(LED_GREEN, HIGH);
		// digitalWrite(LED_BLUE, LOW);
		// disable_interrupts();
		if (!Fifo.enQueue(false))
		{
			// enable_interrupts();
			MYLOG("REED", "FiFo full");
			return;
		}
		// enable_interrupts();
	}
	else
	{
		// digitalWrite(LED_GREEN, LOW);
		// digitalWrite(LED_BLUE, HIGH);
		// disable_interrupts();
		if (!Fifo.enQueue(true))
		{
			// enable_interrupts();
			MYLOG("REED", "FiFo full");
			return;
		}
		// enable_interrupts();
	}

	MYLOG("REED", "Added event to queue");
	// if (Fifo.getSize() == 1)
	if (handler_available)
	{
		MYLOG("REED", "Start event handler");
		// Wake the switch handler
		// api.system.timer.start(RAK_TIMER_2, 50, NULL);
		handler_available = false;
		handle_rak13011(NULL);
	}
}

bool init_rak13011(void)
{
	// Create a timers for handling the events
	api.system.timer.create(RAK_TIMER_2, switch_bounce_check, RAK_TIMER_ONESHOT);
	api.system.timer.create(RAK_TIMER_3, handle_rak13011, RAK_TIMER_ONESHOT);

	MYLOG("REED", "Initialize Interrupt on pin %d", SW_INT_PIN);
	pinMode(SW_INT_PIN, INPUT);
	attachInterrupt(SW_INT_PIN, switch_int_handler, CHANGE);
	MYLOG("REED", "Interrupt Initialized ");

	return true;
}

void handle_rak13011(void *)
{
	if (!tx_active)
	{
		// Reset automatic interval sending if active
		if (g_custom_parameters.send_interval != 0)
		{
			// Restart a timer
			api.system.timer.stop(RAK_TIMER_0);
			api.system.timer.start(RAK_TIMER_0, g_custom_parameters.send_interval, NULL);
		}

		// Clear payload
		g_solution_data.reset();

		noInterrupts();
		g_solution_data.addPresence(LPP_CHANNEL_SWITCH, !Fifo.deQueue() ? 0 : 1);
		interrupts();

		// Add battery voltage
		g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

		// Send the packet
		send_packet();
	}
	else
	{
		MYLOG("REED", "TX still active");
		// Fifo.deQueue();
	}

	MYLOG("REED", "Queue entries = %d", Fifo.getSize());
	if (!Fifo.isEmpty())
	{
		// Event queue is not empty. Trigger next packet in 5 seconds
		api.system.timer.start(RAK_TIMER_3, 5000, NULL);
	}
	else
	{
		handler_available = true;
		Serial.println("Queue is empty");
	}
}
