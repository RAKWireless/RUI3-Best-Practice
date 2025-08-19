/**
 * @file RAK14000_epd.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization and functions for EPD display
 * @version 0.1
 * @date 2022-06-25
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "../inc/app.h"

#include "../inc/RAK14000_epd.h"

char disp_text[60];

unsigned char image[4000];
EPD_213_BW epd;
Paint paint(image, 122, 250);

uint16_t bg_color = UNCOLORED;
uint16_t txt_color = COLORED;

void status_rak14000(void)
{
	if (!api.lorawan.njs.get())
	{
		paint.DrawFilledRectangle(240, 0, 249, 5, txt_color);
		paint.DrawFilledRectangle(225, 7, 235, 12, txt_color);
	}
	else
	{
		paint.DrawFilledRectangle(240, 0, 249, 5, txt_color);
		paint.DrawFilledRectangle(235, 5, 245, 10, txt_color);
	}
}

void init_rak14000(void)
{
	pinMode(POWER_ENABLE, INPUT_PULLUP);
	digitalWrite(POWER_ENABLE, HIGH);

	// set left button interrupt
	pinMode(LEFT_BUTTON, INPUT_PULLUP);

	// set middle button interrupt
	pinMode(MIDDLE_BUTTON, INPUT_PULLUP);

	// set right button interrupt
	pinMode(RIGHT_BUTTON, INPUT_PULLUP);

	clear_rak14000();
	paint.drawBitmap(5, 25, (uint8_t *)rak_img, 59, 56);
	rak14000_text(35, 5, "RAKWireless", (uint16_t)txt_color, 2);
	rak14000_text(70, 25, "IoT Made Easy", (uint16_t)txt_color, 2);
	rak14000_text(0, 104, "OTAA", (uint16_t)txt_color, 2);
	if (api.lorawan.njs.get() == 0)
	{
		rak14000_text(100, 104, "----", (uint16_t)txt_color, 2);
	}
	else
	{
		rak14000_text(100, 104, "SEND", (uint16_t)txt_color, 2);
	}
	rak14000_text(200, 104, "ABP", (uint16_t)txt_color, 2);

	if (api.lorawan.njm.get())
	{
		paint.DrawRectangle(0, 103, 60, 120, txt_color);
		paint.DrawRectangle(0, 102, 61, 121, txt_color);
	}
	else
	{
		paint.DrawRectangle(199, 103, 250, 120, txt_color);
		paint.DrawRectangle(198, 102, 250, 121, txt_color);
	}

	status_rak14000();

	if (tx_finished)
	{
		snprintf(disp_text, 29, "TX finished");
	}
	else
	{
		if (api.lorawan.njs.get() != 0)
		{
			rak14000_text(70, 50, "Joined", (uint16_t)txt_color, 2);
		}
		else
		{
			rak14000_text(70, 50, "Not joined", (uint16_t)txt_color, 2);
		}
	}
	tx_finished = false;

	epd.Display(image);
	MYLOG("EPD", "Init RAK14000 finished");
}

/**
   @brief Write a text on the display

   @param x x position to start
   @param y y position to start
   @param text text to write
   @param text_color color of text
   @param text_size size of text
*/
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size)
{
	sFONT *use_font;
	// switch (text_size)
	// {
	// case 1:
	// 	use_font = &Font16;
	// 	break;
	// default:
	use_font = &Font20;
	// 	break;
	// }

	paint.DrawStringAt(x, y, text, use_font, COLORED);
}

void clear_rak14000(void)
{
	paint.SetRotate(ROTATE_270);
	epd.Init(FULL);
	// epd.Clear();
	paint.Clear(UNCOLORED);
}

void refresh_rak14000(void)
{
	MYLOG("EPD", "Refresh RAK14000");
	// Clear display buffer
	clear_rak14000();

	paint.Clear(UNCOLORED);
	paint.drawBitmap(5, 25, (uint8_t *)rak_img, 59, 56);
	rak14000_text(35, 5, "RAKWireless", (uint16_t)txt_color, 2);
	rak14000_text(70, 25, "IoT Made Easy", (uint16_t)txt_color, 2);
	rak14000_text(0, 104, "OTAA", (uint16_t)txt_color, 2);
	if (api.lorawan.njs.get() != 0)
	{
		rak14000_text(100, 104, "SEND", (uint16_t)txt_color, 2);
	}
	else
	{
		rak14000_text(100, 104, "----", (uint16_t)txt_color, 2);
	}
	rak14000_text(200, 104, "ABP", (uint16_t)txt_color, 2);

	if (api.lorawan.njm.get())
	{
		paint.DrawRectangle(0, 103, 60, 120, txt_color);
		paint.DrawRectangle(0, 102, 61, 121, txt_color);
	}
	else
	{
		paint.DrawRectangle(199, 103, 250, 120, txt_color);
		paint.DrawRectangle(198, 102, 250, 121, txt_color);
	}

	status_rak14000();

	if (tx_started)
	{
		tx_started = false;
		snprintf(disp_text, 29, "TX started");
	}
	else if (tx_finished)
	{
		tx_finished = false;
		snprintf(disp_text, 29, "TX finished");
	}
	else
	{
		if (is_joining)
		{
			is_joining = false;
			snprintf(disp_text, 29, "Joining");
		}
		else
		{
			if (api.lorawan.njs.get() != 0)
			{
				snprintf(disp_text, 29, "Joined");
			}
			else
			{
				snprintf(disp_text, 29, "Not Joined");
			}
		}
	}
	rak14000_text(70, 50, disp_text, txt_color, 2);

	if (has_rx)
	{
		has_rx = false;
		snprintf(disp_text, 29, "RX:%02X%02X%2X%02X%2X", last_rx_buff[0],
				 last_rx_buff[1], last_rx_buff[2], last_rx_buff[3], last_rx_buff[4]);
		rak14000_text(70, 75, disp_text, txt_color, 2);
	}
	epd.Init(FULL);
	epd.Display(image);
}
