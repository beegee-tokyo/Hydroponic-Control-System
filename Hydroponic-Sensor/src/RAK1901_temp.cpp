/**
 * @file RAK1901_temp.cpp
 * @author Bernd Giesecke (bernd.giesecke@rakwireless.com)
 * @brief Initialize and read data from SHTC3 sensor
 * @version 0.3
 * @date 2024-02-21
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "main.h"
#include "SparkFun_SHTC3.h"

/** Sensor instance */
SHTC3 shtc3;

/** Last temperature read */
float g_last_temp = 0;
/** Last humidity read */
float g_last_humid = 0;

/**
 * @brief Initialize the temperature and humidity sensor
 *
 * @return true if initialization is ok
 * @return false if sensor could not be found
 */
bool init_rak1901(void)
{
	if (shtc3.begin(Wire) != SHTC3_Status_Nominal)
	{
		MYLOG("T_H", "Could not initialize SHTC3");
		return false;
	}
	shutdown_rak1901();
	return true;
}

/**
 * @brief Read the temperature and humidity values
 *     Data is added to Cayenne LPP payload as channel
 *     LPP_CHANNEL_HUMID, LPP_CHANNEL_TEMP
 *
 */
void read_rak1901(void)
{
	MYLOG("T_H", "Reading SHTC3");
	shtc3.update();

	if (shtc3.lastStatus == SHTC3_Status_Nominal)
	{
		int16_t temp_int = (int16_t)(shtc3.toDegC() * 10.0);
		uint16_t humid_int = (uint16_t)(shtc3.toPercent() * 2);

		MYLOG("T_H", "T: %.2f H: %.2f", (float)temp_int / 10.0, (float)humid_int / 2.0);

		payload.addRelativeHumidity(LPP_CHANNEL_HUMID, shtc3.toPercent());
		payload.addTemperature(LPP_CHANNEL_TEMP, shtc3.toDegC());
		g_last_temp = shtc3.toDegC();
		g_last_humid = shtc3.toPercent();
	}
	else
	{
		MYLOG("T_H", "Reading SHTC3 failed");
		g_last_temp = 0.0;
		g_last_humid = 0.0;
	}
}

/**
 * @brief Returns the latest values from the sensor
 *        or starts a new reading
 *
 * @param values array for temperature [0] and humidity [1]
 */
void get_rak1901_values(float *values)
{
	values[0] = g_last_temp;
	values[1] = g_last_humid;
	values[2] = 1000.0; // no sensor available
	return;
}

/**
 * @brief Wake up RAK1901 from sleep
 *
 */
void startup_rak1901(void)
{
	shtc3.wake(false);
	shtc3.update();
}

/**
 * @brief Put the RAK1901 into sleep mode
 *
 */
void shutdown_rak1901(void)
{
	shtc3.sleep(true);
	delay(250);
}