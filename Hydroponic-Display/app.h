/**
 * @file app.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Defines, includes, global definitions
 * @version 0.1
 * @date 2023-05-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <Arduino.h>

// Debug
// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                  \
	do                                   \
	{                                    \
		if (tag)                         \
			Serial.printf("[%s] ", tag); \
		Serial.printf(__VA_ARGS__);      \
		Serial.printf("\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

// Forward declarations
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool get_at_setting(void);
bool save_at_setting(void);

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1	// Base Board

// EPD
void status_rak14000(void);
void init_rak14000(void);
void rak14000_text(int16_t x, int16_t y, char *text, uint16_t text_color, uint32_t text_size);
void rak14000_logo(int16_t x, int16_t y);
void clear_rak14000(void);
void refresh_rak14000(void);

extern float ph_level;
extern uint16_t tds_level;
extern float temp_level;
extern float humid_level;
extern float water_level;
extern bool has_ph;
extern bool has_tds;
extern bool has_temp;
extern bool has_humid;
extern bool has_water_level;
#define POWER_ENABLE WB_IO2
