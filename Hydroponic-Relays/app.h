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
		if (tag)                         \
			Serial6.printf("[%s] ", tag); \
		Serial6.printf(__VA_ARGS__);      \
		Serial6.printf("\n");             \
	} while (0);                         \
	delay(100)
#else
#define MYLOG(...)
#endif

// Forward declarations
void send_packet(void);
bool init_status_at(void);
bool init_interval_at(void);
bool init_pump_at(void);
bool get_at_setting(void);
bool save_at_setting(void);
uint8_t get_min_dr(uint16_t region, uint16_t payload_size);
extern volatile uint8_t relay_pump_status;

/** Custom flash parameters structure */
struct custom_param_s
{
	uint8_t valid_flag = 0xAA;
	uint32_t send_interval = 0;
	uint32_t pump_on_time = 0;
	uint32_t pump_off_time = 0;
};

/** Custom flash parameters */
extern custom_param_s custom_parameters;

// LoRaWAN stuff
#include "wisblock_cayenne.h"
// Cayenne LPP Channel numbers per sensor value
#define LPP_CHANNEL_BATT 1	// Base Board
#define LPP_CHANNEL_WATER 2 // Status water relay
#define LPP_CHANNEL_NUTRITION 3 // Status nutrition relay

// Other definitions
#ifndef WB_IO7
#define WB_IO7 WB_QSPI_DIO2
#endif
#define RELAY_PUMP WB_IO4
#define RELAY_NUTRITION WB_IO7

#define WATER_PUMP 0
#define NUTRITION_VALVE 1

#define LONG 2

// Downlink Commands
#define REM_TIME 0
