/**
 * @file main.h
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Defines and includes
 * @version 0.1
 * @date 2024-03-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <Arduino.h>
#include <WisBlock-API-V2.h>

// Debug output set to 0 to disable app debug output
#ifndef MY_DEBUG
#define MY_DEBUG 1
#endif

#if MY_DEBUG > 0
#define MYLOG(tag, ...)                     \
	do                                      \
	{                                       \
		if (tag)                            \
			PRINTF("[%s] ", tag);           \
		PRINTF(__VA_ARGS__);                \
		PRINTF("\n");                       \
		if (g_ble_uart_is_connected)        \
		{                                   \
			g_ble_uart.printf(__VA_ARGS__); \
			g_ble_uart.printf("\n");        \
		}                                   \
	} while (0)
#else
#define MYLOG(...)
#endif

/** Define the version of your SW */
#define SW_VERSION_1 1 // major version increase on API change / not backwards compatible
#define SW_VERSION_2 0 // minor version increase on API change / backward compatible
#define SW_VERSION_3 0 // patch version increase on bugfix, no affect on API

/** Application function definitions */
void setup_app(void);
bool init_app(void);
void app_event_handler(void);
void ble_data_handler(void) __attribute__((weak));
void lora_data_handler(void);

// TDS sensor
void init_tds(void);
bool read_tds(bool add_to_payload);

// Temperature sensor
bool init_rak1901(void);
void read_rak1901(void);
void get_rak1901_values(float *values);
void startup_rak1901(void);
void shutdown_rak1901(void);
extern bool has_rak1901;

// ModBus sensors
void init_rak5802(void);
bool read_rak5802(uint8_t sensor_type);

#define LPP_TDS 64 // TDS sensor
#define LPP_PH 65  // pH sensor

// Hydroponic settings
struct s_hydro_settings
{
	uint16_t valid_mark = 0xAA55;	   // Validity marker
	uint32_t nutrition_level = 1000;   // Default nutrition level
	uint16_t calibration_factor = 100; // Calibration factor (x100)
};

// Payload
extern WisCayenne payload;

// Downlink commands
#define NUTRITION_VALUE 1
#define CALIB_FACTOR 2

// Custom AT commands
int at_set_nutrition(char *str);
int at_query_nutrition(void);
bool read_hydro_settings(void);
void save_hydro_settings(void);
void init_custom_at(void);
extern s_hydro_settings g_hydro_settings;