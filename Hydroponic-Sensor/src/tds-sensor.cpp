/**
 * @file tds-sensor.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialize and read TDS sensor
 * @version 0.1
 * @date 2024-03-27
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "main.h"

#define TdsSensorPin WB_A1
#define VREF 3.0		   // analog reference voltage(Volt) of the ADC
#define SCOUNT 50		   // sum of sample point
int analog_buffer[SCOUNT]; // store the analog value in the array, read from ADC
int analog_bufferTemp[SCOUNT];
int analog_bufferIndex = 0;
float average_voltage = 0, tds_value = 0, temperature = 25;

int getMedianNum(int bArray[], int iFilterLen);

/**
 * @brief Initialize the TDS sensor
 *
 */
void init_tds(void)
{
	pinMode(TdsSensorPin, INPUT);
	analogOversampling(128);
}

/**
 * @brief Read from the TDS sensor
 *
 * @param add_to_payload true ==> add values to payload false ==> ignore values
 * @return true Readings ok
 * @return false Readings too low, probably no sensor
 */
bool read_tds(bool add_to_payload)
{
	if (has_rak1901)
	{
		float t_h_values[3];
		get_rak1901_values(t_h_values);
		temperature = t_h_values[0];
		MYLOG("TDS", "Using sensor temperature %.2f ^C", temperature);
	}

	for (int readings = 0; readings < SCOUNT; readings++)
	{
		analog_buffer[readings] = analogRead(TdsSensorPin); // read the analog value and store into the buffer
		delay(100);
	}

	average_voltage = getMedianNum(analog_buffer, SCOUNT) * (float)VREF / 4096.0;																										  // 12bit resolution
	MYLOG("TDS", "Avg Voltage %.2fV", average_voltage);																																	  // 16 bit resolution
	float compensation_coefficient = 1.0 + 0.02 * (temperature - 25.0);																													  // temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
	float compensation_voltage = average_voltage / compensation_coefficient;																											  // temperature compensation
	tds_value = (133.42 * compensation_voltage * compensation_voltage * compensation_voltage - 255.86 * compensation_voltage * compensation_voltage + 857.39 * compensation_voltage) * ((float)g_hydro_settings.calibration_factor/100.0); // 0.5 in code, 1 in library ????? convert voltage value to tds value

	MYLOG("TDS", "Avg TDS %.2fppm", tds_value);
	MYLOG("TDS", "Req TDS %ldppm", g_hydro_settings.nutrition_level);
	MYLOG("TDS", "Nutrion valve %s", (uint32_t)tds_value < g_hydro_settings.nutrition_level ? "ON" : "OFF");
	MYLOG("TDS", "Est EC %.2fus/cm", tds_value * 2.0);

	if (add_to_payload)
	{
		// Add to payload
		payload.addConcentration(LPP_TDS, tds_value);
		// Check if nutrition level is below threshold
		payload.addPresence(LPP_TDS, ((uint32_t)tds_value < g_hydro_settings.nutrition_level ? true : false));
	}

	if (tds_value >= 50.0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/**
 * @brief Calculate the Median Number
 *
 * @param bArray
 * @param iFilterLen
 * @return int
 */
int getMedianNum(int bArray[], int iFilterLen)
{
	int bTab[iFilterLen];
	for (byte i = 0; i < iFilterLen; i++)
		bTab[i] = bArray[i];
	int i, j, bTemp;
	for (j = 0; j < iFilterLen - 1; j++)
	{
		for (i = 0; i < iFilterLen - j - 1; i++)
		{
			if (bTab[i] > bTab[i + 1])
			{
				bTemp = bTab[i];
				bTab[i] = bTab[i + 1];
				bTab[i + 1] = bTemp;
			}
		}
	}
	if ((iFilterLen & 1) > 0)
		bTemp = bTab[(iFilterLen - 1) / 2];
	else
		bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
	return bTemp;
}

/**++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/**++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/**++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

// #include <GravityTDS.h>

// GravityTDS tds_sensor;

// /**
//  * @brief Initialize the TDS sensor
//  *
//  */
// void init_tds(void)
// {
// 	pinMode(WB_A1, INPUT);
// 	analogOversampling(128);
// 	tds_sensor.setPin(WB_A1);
// 	tds_sensor.setAref(3.0);
// 	tds_sensor.setAdcRange(4096);
// 	tds_sensor.begin();
// }

// /**
//  * @brief Read from the TDS sensor
//  *
//  * @param add_to_payload true ==> add values to payload false ==> ignore values
//  * @return true Readings ok
//  * @return false Readings too low, probably no sensor
//  */
// bool read_tds(bool add_to_payload)
// {
// 	if (has_rak1901)
// 	{
// 		float t_h_values[3];
// 		get_rak1901_values(t_h_values);
// 		tds_sensor.setTemperature(t_h_values[0]);
// 		MYLOG("TDS", "Using sensor temperature %.2f ^C", t_h_values[0]);
// 	}

// 	double sampled_tds;
// 	for (int readings = 0; readings < 10; readings++)
// 	{
// 		tds_sensor.update();
// 		sampled_tds += (double)tds_sensor.getTdsValue();
// 		delay(100);
// 	}

// 	float tds_value = (float)(sampled_tds / 10.0);

// 	MYLOG("TDS", "Avg TDS %.2fppm", tds_value);
// 	MYLOG("TDS", "Req TDS %ldppm", g_hydro_settings.nutrition_level);
// 	MYLOG("TDS", "Nutrion valve %s", (uint32_t)tds_value < g_hydro_settings.nutrition_level ? "ON" : "OFF");
// 	MYLOG("TDS", "Est EC %.2fus/cm", tds_value * 2.0);

// 	if (add_to_payload)
// 	{
// 		// Add to payload
// 		payload.addConcentration(LPP_TDS, tds_value);
// 		// Check if nutrition level is below threshold
// 		payload.addPresence(LPP_TDS, ((uint32_t)tds_value < g_hydro_settings.nutrition_level ? true : false));
// 	}

// 	if (tds_value >= 50.0)
// 	{
// 		return true;
// 	}
// 	else
// 	{
// 		return false;
// 	}
// }
