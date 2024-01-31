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
#include "app.h"

#include <rak14000.h> //Click here to get the library: http://librarymanager/All#RAK14000

#include "RAK14000_epd.h"

#define LEFT_BUTTON WB_IO6
#define MIDDLE_BUTTON WB_IO5
#define RIGHT_BUTTON WB_IO3

float last_temp_value = 0.0;
float last_humid_value = 0.0;
float last_baro_value = 0.0;
float last_light_value = 0.0;
char disp_text[60];

unsigned char image[4000];
EPD_213_BW epd;
Paint paint(image, 122, 250);

uint16_t bg_color = UNCOLORED;
uint16_t txt_color = COLORED;

void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);

void status_rak14000(void)
{
	if (!api.lorawan.njs.get())
	{
		paint.DrawRectangle(240, 0, 249, 5, txt_color);
		paint.DrawRectangle(225, 7, 235, 12, txt_color);
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
	paint.drawBitmap(5, 5, (uint8_t *)rak_img, 59, 56);
	rak14000_text(60, 65, "RAKWireless", (uint16_t)txt_color, 2);
	rak14000_text(60, 85, "IoT Made Easy", (uint16_t)txt_color, 2);

	status_rak14000();

	epd.Display(image);
	delay(500);
	digitalWrite(POWER_ENABLE, LOW);
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
	// Clear display buffer
	clear_rak14000();

	paint.Clear(UNCOLORED);
	rak14000_text(0, 4, "RUI3 Environment", txt_color, 1);

	status_rak14000();

	temp_rak14000();
	humid_rak14000();
	baro_rak14000();

	if ((has_1903) || (has_12010))
	{
		light_rak14000();
	}

	epd.Init(FULL);
	epd.Display(image);
}

void set_temp_rak14000(float temp_value)
{
	// MYLOG("EPD", "Temp set to %.2f", temp_value);

	// Save Temperature Value
	last_temp_value = temp_value;
}

void set_humid_rak14000(float humid_value)
{
	// MYLOG("EPD", "Humid set to %.2f", humid_value);

	// Save Humidity Value
	last_humid_value = humid_value;
}

void set_baro_rak14000(float baro_value)
{
	// MYLOG("EPD", "Baro set to %.2f", baro_value);

	// Save Barometer Value
	last_baro_value = baro_value;
}

void set_light_rak14000(float lux_value)
{
	// MYLOG("EPD", "Light set to %.2f", lux_value);

	// Save Light Value
	last_light_value = lux_value;
}

void temp_rak14000(void)
{
	uint16_t x_text = 0;
	uint16_t y_text = 29;
	uint16_t s_text = 2;
	if ((has_1903) || (has_12010))
	{
		y_text = 26;
	}
	else
	{
		y_text = 40;
	}

	// Write value
	snprintf(disp_text, 29, "Temp:  %.2f", last_temp_value);
	rak14000_text(x_text, y_text, disp_text, txt_color, s_text);
	snprintf(disp_text, 29, "~C");
	rak14000_text(190, y_text, disp_text, txt_color, s_text);
}

void humid_rak14000(void)
{
	uint16_t x_text = 0;
	uint16_t y_text = 54;
	uint16_t s_text = 2;
	if ((has_1903) || (has_12010))
	{
		y_text = 52;
	}
	else
	{
		y_text = 70;
	}

	// Write value
	// display.fillRect(x_text, y_text, w_text, h_text, bg_color);
	snprintf(disp_text, 29, "Humid: %.2f", last_humid_value);
	rak14000_text(x_text, y_text, disp_text, txt_color, s_text);
	snprintf(disp_text, 29, "%%RH");
	rak14000_text(190, y_text, disp_text, txt_color, s_text);
}

void baro_rak14000(void)
{
	uint16_t x_text = 0;
	uint16_t y_text = 76;
	uint16_t s_text = 2;
	if ((has_1903) || (has_12010))
	{
		y_text = 76;
	}
	else
	{
		y_text = 100;
	}

	// Write value
	snprintf(disp_text, 29, "Baro:  %.1f", last_baro_value);
	rak14000_text(x_text, y_text, disp_text, txt_color, s_text);
	snprintf(disp_text, 29, "mBar");
	rak14000_text(190, y_text, disp_text, txt_color, s_text);
}

void light_rak14000(void)
{
	if ((has_1903) || (has_12010))
	{
		uint16_t x_text = 0;
		uint16_t y_text = 100;
		uint16_t s_text = 2;

		// Write value
		snprintf(disp_text, 29, "Light: %.1f", last_light_value);
		rak14000_text(x_text, y_text, disp_text, txt_color, s_text);
		snprintf(disp_text, 29, "Lux");
		rak14000_text(190, y_text, disp_text, txt_color, s_text);
	}
}
