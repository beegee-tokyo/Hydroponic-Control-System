/**
 * @file main.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Low power test
 * @version 0.1
 * @date 2023-02-14
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"

/** Send Fail counter **/
uint8_t send_fail = 0;

/** Set the device name, max length is 16 characters */
char g_ble_dev_name[17] = "RAK-HYDRO-SENS";

/** Payload */
WisCayenne payload(255);

/** Flag if temperature sensor was found */
bool has_rak1901 = false;
/** Flag if pH sensor was found */
bool has_ph_sensor = false;
/** Flag if TDS sensor was found */
bool has_tds_sensor = false;

/**
 * @brief Initial setup of the application (before LoRaWAN and BLE setup)
 *
 */
void setup_app(void)
{
	Serial.begin(115200);
	time_t serial_timeout = millis();
	// On nRF52840 the USB serial is not available immediately
	while (!Serial)
	{
		if ((millis() - serial_timeout) < 5000)
		{
			delay(100);
			digitalWrite(LED_GREEN, !digitalRead(LED_GREEN));
		}
		else
		{
			break;
		}
	}
	digitalWrite(LED_GREEN, LOW);

	// Set firmware version
	api_set_version(SW_VERSION_1, SW_VERSION_2, SW_VERSION_3);

	MYLOG("APP", "Setup application");
	g_enable_ble = true;
}

/**
 * @brief Final setup of application  (after LoRaWAN and BLE setup)
 *
 * @return true
 * @return false
 */
bool init_app(void)
{
	MYLOG("APP", "Initialize application");
	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);
	delay(500);

	init_custom_at();
	read_hydro_settings();

	Wire.begin();

	// Initialize RAK1901 temperature sensor
	has_rak1901 = init_rak1901();
	startup_rak1901();
	delay(500);
	read_rak1901();
	shutdown_rak1901();

	// Initialize the TDS sensor
	init_tds();
	has_tds_sensor = read_tds(false);

	// Initialize the MODbus master (pH sensor)
	init_rak5802();
	has_ph_sensor = read_rak5802(LPP_PH);

	digitalWrite(WB_IO2, LOW);

	restart_advertising(0);
	return true;
}

/**
 * @brief Handle events
 * 		Events can be
 * 		- timer (setup with AT+SENDINT=xxx)
 * 		- interrupt events
 * 		- wake-up signals from other tasks
 */
void app_event_handler(void)
{
	// Timer triggered event
	if ((g_task_event_type & STATUS) == STATUS)
	{
		g_task_event_type &= N_STATUS;
		MYLOG("APP", "Timer wakeup");

		if (has_rak1901)
		{
			startup_rak1901();
		}

		digitalWrite(WB_IO2, HIGH);
		delay(500);

		// Prepare payload
		payload.reset();

		// Read temperature and humidity
		if (has_rak1901)
		{
			read_rak1901();
			shutdown_rak1901();
		}

		// Read TDS sensor
		if (has_tds_sensor)
		{
			read_tds(true);
		}

		// Read pH sensor
		if (has_ph_sensor)
		{
			read_rak5802(LPP_PH);
		}

		// Get Battery status
		float batt_level_f = 0.0;
		for (int readings = 0; readings < 10; readings++)
		{
			batt_level_f += read_batt();
		}
		batt_level_f = batt_level_f / 10;
		payload.addVoltage(LPP_CHANNEL_BATT, batt_level_f / 1000.0);

		if (g_lorawan_settings.lorawan_enable)
		{
			if (g_lpwan_has_joined)
			{

				lmh_error_status result = send_lora_packet(payload.getBuffer(), payload.getSize());
				switch (result)
				{
				case LMH_SUCCESS:
					MYLOG("APP", "Packet enqueued");
					break;
				case LMH_BUSY:
					MYLOG("APP", "LoRa transceiver is busy");
					break;
				case LMH_ERROR:
					MYLOG("APP", "Packet error, too big to send with current DR");
					break;
				}
			}
			else
			{
				MYLOG("APP", "Network not joined, skip sending");
			}
		}
		else
		{
			send_p2p_packet(payload.getBuffer(), payload.getSize());
		}
		digitalWrite(WB_IO2, LOW);
	}
}

/**
 * @brief Handle BLE events
 *
 */
void ble_data_handler(void)
{
	if (g_enable_ble)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo BLE UART data arrived
		/// \todo or forward them to the AT command interpreter
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		if ((g_task_event_type & BLE_DATA) == BLE_DATA)
		{
			MYLOG("AT", "RECEIVED BLE");
			// BLE UART data arrived
			// in this example we forward it to the AT command interpreter
			g_task_event_type &= N_BLE_DATA;

			while (g_ble_uart.available() > 0)
			{
				at_serial_input(uint8_t(g_ble_uart.read()));
				delay(5);
			}
			at_serial_input(uint8_t('\n'));
		}
	}
}

/**
 * @brief Handle LoRa events
 *
 */
void lora_data_handler(void)
{
	// LoRa Join finished handling
	if ((g_task_event_type & LORA_JOIN_FIN) == LORA_JOIN_FIN)
	{
		g_task_event_type &= N_LORA_JOIN_FIN;
		if (g_join_result)
		{
			MYLOG("APP", "Successfully joined network");
		}
		else
		{
			MYLOG("APP", "Join network failed");
			/// \todo here join could be restarted.
			// lmh_join();
		}
	}

	// LoRa data handling
	if ((g_task_event_type & LORA_DATA) == LORA_DATA)
	{
		/**************************************************************/
		/**************************************************************/
		/// \todo LoRa data arrived
		/// \todo parse them here
		/**************************************************************/
		/**************************************************************/
		g_task_event_type &= N_LORA_DATA;
		MYLOG("RX_CB", "Received package over LoRa");
		MYLOG("RX_CB", "Last RSSI %d", g_last_rssi);

		char log_buff[g_rx_data_len * 3] = {0};
		uint8_t log_idx = 0;
		for (int idx = 0; idx < g_rx_data_len; idx++)
		{
			sprintf(&log_buff[log_idx], "%02X ", g_rx_lora_data[idx]);
			log_idx += 3;
		}
		MYLOG("RX_CB", "%s", log_buff);

		if (g_last_fport == 11)
		{ 
			// Check data size
			if (g_rx_data_len > 3)
			{
				// Check for valid setup sequence
				if ((g_rx_lora_data[0] == 0xAA) && (g_rx_lora_data[1] == 0x55))
				{
					uint32_t new_value_1 = 0;
					uint32_t old_value_1 = 0;
					uint32_t new_value_2 = 0;
					uint32_t old_value_2 = 0;
					// Check for setup type
					switch (g_rx_lora_data[2])
					{
					case NUTRITION_VALUE:
						if (g_rx_data_len != 7)
						{
							MYLOG("RX_CB", "Wrong setup size");
						}

						old_value_1 = g_hydro_settings.nutrition_level;
						new_value_1 = g_rx_lora_data[3] << 24;
						new_value_1 |= g_rx_lora_data[4] << 16;
						new_value_1 |= g_rx_lora_data[5] << 8;
						new_value_1 |= g_rx_lora_data[6] << 0;
						MYLOG("RX_CB", "New nutrition level %ld", new_value_1);
						// Save custom settings if needed
						if (old_value_1 != new_value_1)
						{
							g_hydro_settings.nutrition_level = new_value_1;
							save_hydro_settings();
						}
						break;
						case CALIB_FACTOR:
							if (g_rx_data_len != 5)
							{
								MYLOG("RX_CB", "Wrong setup size");
							}

							old_value_1 = g_hydro_settings.calibration_factor;
							new_value_1 = g_rx_lora_data[3] << 8;
							new_value_1 |= g_rx_lora_data[4] << 0;
							MYLOG("RX_CB", "New calibration factor %ld", new_value_1);
							// Save custom settings if needed
							if (old_value_1 != new_value_1)
							{
								g_hydro_settings.calibration_factor = new_value_1;
								save_hydro_settings();
							}
							break;
					default:
						MYLOG("RX_CB", "Wrong setup type");
						break;
					}
				}
				else
				{
					MYLOG("RX_CB", "Wrong format");
				}
			}
			else
			{
				MYLOG("RX_CB", "Wrong size");
			}
		}
		else
		{
			MYLOG("RX_CB", "Wrong fPort");
		}
	}

	// LoRa TX finished handling
	if ((g_task_event_type & LORA_TX_FIN) == LORA_TX_FIN)
	{
		g_task_event_type &= N_LORA_TX_FIN;

		if (g_lorawan_settings.lorawan_enable)
		{
			if (g_lorawan_settings.confirmed_msg_enabled == LMH_UNCONFIRMED_MSG)
			{
				MYLOG("APP", "LPWAN TX cycle finished");
			}
			else
			{
				MYLOG("APP", "LPWAN TX cycle %s", g_rx_fin_result ? "finished ACK" : "failed NAK");
			}
			if (!g_rx_fin_result)
			{
				// Increase fail send counter
				send_fail++;

				if (send_fail == 10)
				{
					// Too many failed sendings, reset node and try to rejoin
					delay(100);
					api_reset();
				}
			}
		}
		else
		{
			MYLOG("APP", "P2P TX finished");
		}
	}
}
