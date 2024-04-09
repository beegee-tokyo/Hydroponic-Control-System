/**
 * @file custom_at_cmd.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief AT command extension for Hydroponic sensor settings
 * @version 0.1
 * @date 2024-03-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "main.h"

/** Structure for saved Blues Notecard settings */
s_hydro_settings g_hydro_settings;

#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
using namespace Adafruit_LittleFS_Namespace;

/** Filename to save Blues settings */
static const char hydro_file_name[] = "HYDRO";

/** File to save battery check status */
File this_file(InternalFS);

#define REQ_PRINTF(...)                     \
	do                                      \
	{                                       \
		PRINTF(__VA_ARGS__);                \
		PRINTF("\n");                       \
		Serial.flush();                     \
		if (g_ble_uart_is_connected)        \
		{                                   \
			g_ble_uart.printf(__VA_ARGS__); \
			g_ble_uart.printf("\n");        \
		}                                   \
	} while (0)

/**
 * @brief Set nutrition level
 *
 * @param str Nutrition level as Hex String
 * @return int AT_SUCCESS if ok, AT_ERRNO_PARA_FAIL if invalid value
 */
int at_set_nutrition(char *str)
{
	long new_nutrition_level = strtoul(str, NULL, 0);

	if (new_nutrition_level < 0)
	{
		return AT_ERRNO_PARA_VAL;
	}

	if (g_hydro_settings.nutrition_level != (uint32_t)new_nutrition_level)
	{
		MYLOG("USR_AT", "Nutrition level has changed");
		g_hydro_settings.nutrition_level = new_nutrition_level;
		save_hydro_settings();
	}
	MYLOG("USR_AT", "New nutrition level = %ld", g_hydro_settings.nutrition_level);
	return AT_SUCCESS;
}

/**
 * @brief Get current nutrition level
 *
 * @return int AT_SUCCESS
 */
int at_query_nutrition(void)
{
	snprintf(g_at_query_buf, ATQUERY_SIZE, "%ld", g_hydro_settings.nutrition_level);
	return AT_SUCCESS;
}

/**
 * @brief Set the calibration factor
 *
 * @param str with the calibration factor (x100) 0 == 0.00 500 == 5.00
 * @return int AT_SUCCESS if ok, AT_ERRNO_PARA_FAIL if invalid value
 */
int at_set_calib(char *str)
{
	long new_calib = strtoul(str, NULL, 0);

	if ((new_calib < 0) || new_calib > 200)
	{
		return AT_ERRNO_PARA_VAL;
	}

	if (g_hydro_settings.calibration_factor != (uint16_t)new_calib)
	{
		MYLOG("USR_AT", "Calibration factor has changed");
		g_hydro_settings.calibration_factor = new_calib;
		save_hydro_settings();
	}
	MYLOG("USR_AT", "New calibration factor = %d", g_hydro_settings.calibration_factor);
	return AT_SUCCESS;
}

/**
 * @brief Get current calibration factor
 *
 * @return int AT_SUCCESS
 */
int at_query_calib(void)
{
	snprintf(g_at_query_buf, ATQUERY_SIZE, "%d (x100)", g_hydro_settings.calibration_factor);
	return AT_SUCCESS;
}

/**
 * @brief Get hydroponic settings
 *
 * @return true Settings found and read
 * @return false ---
 */
bool read_hydro_settings(void)
{
	bool structure_valid = false;
	if (InternalFS.exists(hydro_file_name))
	{
		this_file.open(hydro_file_name, FILE_O_READ);
		this_file.read((void *)&g_hydro_settings.valid_mark, sizeof(s_hydro_settings));
		this_file.close();

		// Check for valid data
		if (g_hydro_settings.valid_mark == 0xAA55)
		{
			structure_valid = true;
			MYLOG("USR_AT", "Valid Hydro settings found, level = %ld", g_hydro_settings.nutrition_level);
			MYLOG("USR_AT", "Valid Hydro settings found, cal factor = %ld", g_hydro_settings.calibration_factor);
		}
		else
		{
			MYLOG("USR_AT", "No valid Hydro settings found");
		}
	}

	if (!structure_valid)
	{
		// No settings file found optional to set defaults (ommitted!)
		MYLOG("USR_AT", "Save settings");
		g_hydro_settings.valid_mark = 0xAA55;	 // Validity marker
		g_hydro_settings.nutrition_level = 1000; // Default nutrition level
		save_hydro_settings();
	}

	return true;
}

/**
 * @brief Save hydroponic settings
 *
 */
void save_hydro_settings(void)
{
	if (InternalFS.exists(hydro_file_name))
	{
		InternalFS.remove(hydro_file_name);
	}

	g_hydro_settings.valid_mark = 0xAA55;
	this_file.open(hydro_file_name, FILE_O_WRITE);
	this_file.write((const char *)&g_hydro_settings.valid_mark, sizeof(s_hydro_settings));
	this_file.close();
	MYLOG("USR_AT", "Saved Hydroponic Settings");
	MYLOG("USR_AT", "Saved nutrition level %ld", g_hydro_settings.nutrition_level);
	MYLOG("USR_AT", "Saved calibration factor %d", g_hydro_settings.calibration_factor);
}

/**
 * @brief List of all available commands with short help and pointer to functions
 *
 */
atcmd_t g_user_at_cmd_new_list[] = {
	/*|    CMD    |     AT+CMD?      |    AT+CMD=?    |  AT+CMD=value |  AT+CMD  | Permissions |*/
	// Module commands
	{"+NUTR", "Set/get the required nutrition level", at_query_nutrition, at_set_nutrition, NULL, "RW"},
	{"+CALIB", "Set/get the sensor calibration factor (x100)", at_query_calib, at_set_calib, NULL, "RW"},
};

/** Number of user defined AT commands */
uint8_t g_user_at_cmd_num = 0;

/** Pointer to the combined user AT command structure */
atcmd_t *g_user_at_cmd_list;

/**
 * @brief Initialize the user defined AT command list
 *
 */
void init_custom_at(void)
{
	// Assign custom AT command list to pointer used by WisBlock API
	g_user_at_cmd_list = g_user_at_cmd_new_list;

	// Add AT commands to structure
	g_user_at_cmd_num += sizeof(g_user_at_cmd_new_list) / sizeof(atcmd_t);
	MYLOG("USR_AT", "Added %d User AT commands", g_user_at_cmd_num);
}
