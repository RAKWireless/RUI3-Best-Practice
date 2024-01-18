/**
 * @file RAK1921_oled.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialization and usage of RAK1921 OLED
 * @version 0.1
 * @date 2022-02-18
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <nRF_SSD1306Wire.h>

void disp_show(void);
void rak1921_show(void);

/** Width of the display in pixel */
#define OLED_WIDTH 128
/** Height of the display in pixel */
#define OLED_HEIGHT 64
/** Height of the status bar in pixel */
#define STATUS_BAR_HEIGHT 11
/** Height of a single line */
#define LINE_HEIGHT 10

/** Number of message lines */
#define NUM_OF_LINES (OLED_HEIGHT - STATUS_BAR_HEIGHT) / LINE_HEIGHT

/** Line buffer for messages */
char disp_buffer[NUM_OF_LINES + 1][32] = {0};

/** Current line used */
uint8_t current_line = 0;

/** Display class using Wire */
SSD1306Wire oled_display(0x3c, PIN_WIRE_SDA, PIN_WIRE_SCL, GEOMETRY_128_64, &Wire);

/**
 * @brief Initialize the display
 *
 * @return true always
 * @return false never
 */
bool init_rak1921(void)
{
	Wire.begin();

	delay(500); // Give display reset some time
	// taskENTER_CRITICAL();
	oled_display.setI2cAutoInit(true);
	oled_display.init();
	oled_display.displayOff();
	oled_display.clear();
	oled_display.displayOn();
	oled_display.setBrightness(200);
	// oled_display.flipScreenVertically();
	oled_display.setContrast(100, 241, 64);
	oled_display.setFont(ArialMT_Plain_10);
	oled_display.display();
	// taskEXIT_CRITICAL();

	return true;
}

/**
 * @brief Write the top line of the display
 */
void rak1921_write_header(char *header_line)
{
	// taskENTER_CRITICAL();
	oled_display.setFont(ArialMT_Plain_10);

	// clear the status bar
	oled_display.setColor(BLACK);
	oled_display.fillRect(0, 0, OLED_WIDTH, STATUS_BAR_HEIGHT);

	oled_display.setColor(WHITE);
	oled_display.setTextAlignment(TEXT_ALIGN_LEFT);

	oled_display.drawString(0, 0, header_line);

	// draw divider line
	oled_display.drawLine(0, 11, 128, 11);
	oled_display.display();
	// taskEXIT_CRITICAL();
}

/**
 * @brief Add a line to the display buffer
 *
 * @param line Pointer to char array with the new line
 */
void rak1921_add_line(char *line)
{
	// taskENTER_CRITICAL();
	if (current_line == NUM_OF_LINES)
	{
		// Display is full, shift text one line up
		for (int idx = 0; idx < NUM_OF_LINES; idx++)
		{
			memcpy(disp_buffer[idx], disp_buffer[idx + 1], 32);
		}
		current_line--;
	}
	snprintf(disp_buffer[current_line], 32, "%s", line);

	if (current_line != NUM_OF_LINES)
	{
		current_line++;
	}

	rak1921_show();
	// taskEXIT_CRITICAL();
}

/**
 * @brief Update display messages
 *
 */
void rak1921_show(void)
{
	oled_display.setColor(BLACK);
	oled_display.fillRect(0, STATUS_BAR_HEIGHT + 1, OLED_WIDTH, OLED_HEIGHT);

	oled_display.setFont(ArialMT_Plain_10);
	oled_display.setColor(WHITE);
	oled_display.setTextAlignment(TEXT_ALIGN_LEFT);
	for (int line = 0; line < current_line; line++)
	{
		oled_display.drawString(0, (line * LINE_HEIGHT) + STATUS_BAR_HEIGHT + 1, disp_buffer[line]);
	}
	oled_display.display();
}

/**
 * @brief Clear the display
 *
 */
void rak1921_clear(void)
{
	oled_display.setColor(BLACK);
	oled_display.fillRect(0, STATUS_BAR_HEIGHT + 1, OLED_WIDTH, OLED_HEIGHT);
	current_line = 0;
}

/**
 * @brief Write a line at given position
 * 
 * @param line line number
 * @param y_pos x position to start
 * @param text String text
 */
void rak1921_write_line(int16_t line, int16_t y_pos, String text)
{
	oled_display.drawString(y_pos, (line * LINE_HEIGHT) + STATUS_BAR_HEIGHT + 1, text);
}

/**
 * @brief Display the buffer
 * 
 */
void rak1921_display(void)
{
	oled_display.display();
}