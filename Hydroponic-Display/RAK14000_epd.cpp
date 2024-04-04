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
	// pinMode(POWER_ENABLE, INPUT_PULLUP);
	digitalWrite(POWER_ENABLE, HIGH);

	// // set left button interrupt
	// pinMode(LEFT_BUTTON, INPUT_PULLUP);

	// // set middle button interrupt
	// pinMode(MIDDLE_BUTTON, INPUT_PULLUP);

	// // set right button interrupt
	// pinMode(RIGHT_BUTTON, INPUT_PULLUP);

	clear_rak14000();
	// paint.drawBitmap(5, 5, (uint8_t *)rak_img, 59, 56);
	rak14000_logo(5, 5);
	rak14000_text(60, 65, "RAKWireless", (uint16_t)txt_color, 2);
	rak14000_text(60, 85, "IoT Made Easy", (uint16_t)txt_color, 2);

	status_rak14000();

	epd.Display(image);
	delay(500);
	digitalWrite(POWER_ENABLE, LOW);
}

void rak14000_logo(int16_t x, int16_t y)
{
	paint.drawBitmap(x, y, (uint8_t *)rak_img, 59, 56);
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
	if (text_size == 1)
	{
		use_font = &Font12;
	}
	else
	{
		use_font = &Font20;
	}
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
	rak14000_text(0, 4, " RAK Hydroponic", txt_color, 2);

	status_rak14000();

	rak14000_logo(0, 31);

	int y_pos = 30;
	int line_space = 20;

	// Check how many sensor values are available
	int lines = 0;
	if (has_tds)
	{
		lines += 1;
	}
	if (has_ph)
	{
		lines += 1;
	}
	if (has_temp)
	{
		lines += 1;
	}
	if (has_humid)
	{
		lines += 1;
	}

	// Setup line spacing
	switch (lines)
	{
	case 1:
	case 2:
		line_space = 40;
		break;
	case 3:
		line_space = 30;
		break;
	case 4:
		line_space = 20;
		break;
	}
	if (has_tds)
	{
		snprintf(disp_text, 59, "TDS: %d ppm", tds_level);
		rak14000_text(70, y_pos, disp_text, (uint16_t)txt_color, 2);
		y_pos += line_space;
	}

	if (has_ph)
	{
		snprintf(disp_text, 59, "pH:  %.1f", ph_level);
		rak14000_text(70, y_pos, disp_text, (uint16_t)txt_color, 2);
		y_pos += line_space;
	}

	if (has_temp)
	{
		snprintf(disp_text, 59, "T:   %.1f~C", temp_level);
		rak14000_text(70, y_pos, disp_text, (uint16_t)txt_color, 2);
		y_pos += line_space;
	}

	if (has_humid)
	{
		snprintf(disp_text, 59, "H:   %.1f%%", humid_level);
		rak14000_text(70, y_pos, disp_text, (uint16_t)txt_color, 2);
		y_pos += line_space;
	}

	if (has_water_level)
	{
		snprintf(disp_text, 59, "Lvl:  %.1f%%", water_level);
		rak14000_text(70, y_pos, disp_text, (uint16_t)txt_color, 2);
		y_pos += line_space;
	}

	snprintf(disp_text, 59, "Batt %.2fV", api.system.bat.get());
	rak14000_text(5, 110, disp_text, (uint16_t)txt_color, 1);

	epd.Init(FULL);
	epd.Display(image);
}
