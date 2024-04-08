/**
 * @file RUI3-Modular.ino
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief RUI3 based code for low power practice
 * @version 0.1
 * @date 2023-03-29
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "app.h"

/** Packet is confirmed/unconfirmed (Set with AT commands) */
bool g_confirmed_mode = false;
/** If confirmed packet, number or retries (Set with AT commands) */
uint8_t g_confirmed_retry = 0;
/** Data rate  (Set with AT commands) */
uint8_t g_data_rate = 3;

/** Time interval to send packets in milliseconds */
uint32_t g_send_repeat_time = 60000;

/** Flag if transmit is active, used by some sensors */
volatile bool tx_active = false;

/** fPort to send packages */
uint8_t set_fPort = 2;

/** LoRaWAN packet */
WisCayenne g_solution_data(255);

/** Relay nutrition status */
volatile uint8_t relay_nutrition_status = LOW;
/** Relay water status */
volatile uint8_t relay_pump_status = LOW;

/** Pump number for timer callback */
uint8_t pump_number = 0;

/**
 * @brief Callback after join request cycle
 *
 * @param status Join result
 */
void joinCallback(int32_t status)
{
	// MYLOG("JOIN-CB", "Join result %d", status);
	if (status != 0)
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - join fail! \r\n");
	}
	else
	{
		MYLOG("JOIN-CB", "LoRaWan OTAA - joined! \r\n");
		digitalWrite(LED_BLUE, LOW);
		// send_handler(NULL);
	}
}

/**
 * @brief LoRaWAN callback after packet was received
 *
 * @param data pointer to structure with the received data
 */
void receiveCallback(SERVICE_LORA_RECEIVE_T *data)
{
	MYLOG("RX-CB", "RX, port %d, DR %d, RSSI %d, SNR %d Size %d", data->Port, data->RxDatarate, data->Rssi, data->Snr, data->BufferSize);
	if (data->Port == 0)
	{
		return;
	}

	for (int i = 0; i < data->BufferSize; i++)
	{
		Serial.printf("%02X", data->Buffer[i]);
	}
	Serial.print("\r\n");
	tx_active = false;

	// Check if it is a control command received on fPort 10
	if (data->Port == 10)
	{
		// Check data size
		if (data->BufferSize == 4)
		{
			// Check for valid command sequence
			if ((data->Buffer[0] == 0xAA) && (data->Buffer[1] == 0x55))
			{
				// Check for valid relay status request
				if ((data->Buffer[3] >= 0) && (data->Buffer[3] < 2))
				{
					// Check which relay
					if (data->Buffer[2] == 1) // Nutrition relay
					{

						// Save the status and call the handler
						if (data->Buffer[3] == 0)
						{
							relay_nutrition_status = LOW;
						}
						else
						{
							relay_nutrition_status = HIGH;
						}
						pump_number = NUTRITION_VALVE;
					}
					else if (data->Buffer[2] == 0) // Water pump relay
					{
						// Save the status and call the handler
						if (data->Buffer[3] == 0)
						{
							relay_pump_status = LOW;
						}
						else
						{
							relay_pump_status = HIGH;
						}
						pump_number = WATER_PUMP;
					}
					else if (data->Buffer[2] == 3) // Nutrition refill
					{
						relay_nutrition_status = LONG;
						pump_number = NUTRITION_VALVE;
					}

					api.system.timer.start(RAK_TIMER_1, 100, &pump_number);
				}
				else
				{
					MYLOG("RX_CB", "Wrong command");
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
	// Check if it is a setup command received on fPort 11
	else if (data->Port == 11)
	{
		// Check data size
		if (data->BufferSize > 3)
		{
			// Check for valid setup sequence
			if ((data->Buffer[0] == 0xAA) && (data->Buffer[1] == 0x55))
			{
				uint32_t new_value_1 = 0;
				uint32_t old_value_1 = 0;
				uint32_t new_value_2 = 0;
				uint32_t old_value_2 = 0;
				// Check for setup type
				switch (data->Buffer[2])
				{
				case REM_TIME:
					if (data->BufferSize != 11)
					{
						MYLOG("RX_CB", "Wrong setup size");
					}

					old_value_1 = custom_parameters.pump_on_time;
					new_value_1 = data->Buffer[3] << 24;
					new_value_1 |= data->Buffer[4] << 16;
					new_value_1 |= data->Buffer[5] << 8;
					new_value_1 |= data->Buffer[6] << 0;
					MYLOG("RX_CB", "New ON time %ld", new_value_1);

					old_value_2 = custom_parameters.pump_off_time;
					new_value_2 = data->Buffer[7] << 24;
					new_value_2 |= data->Buffer[8] << 16;
					new_value_2 |= data->Buffer[9] << 8;
					new_value_2 |= data->Buffer[10] << 0;
					MYLOG("RX_CB", "New OFF time %ld", new_value_2);
					custom_parameters.pump_off_time = new_value_2 * 1000;
					// Save custom settings if needed
					if ((new_value_1 * 1000 != custom_parameters.pump_on_time) || (new_value_2 * 1000 != custom_parameters.pump_off_time))
					{
						custom_parameters.pump_on_time = new_value_1 * 1000;
						custom_parameters.pump_off_time = new_value_2 * 1000;
						save_at_setting();
					}
					// Stop the timer
					api.system.timer.stop(RAK_TIMER_3);
					// Restart the timer if values != 0
					if ((custom_parameters.pump_on_time != 0) && (custom_parameters.pump_off_time != 0))
					{
						relay_pump_status = LOW;
						digitalWrite(RELAY_PUMP, HIGH);
						// Restart the timer
						api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_on_time, (void *)&relay_pump_status);
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

/**
 * @brief Callback for LinkCheck result
 *
 * @param data pointer to structure with the linkcheck result
 */
void linkcheckCallback(SERVICE_LORA_LINKCHECK_T *data)
{
	MYLOG("LC_CB", "%s Margin %d GW# %d RSSI%d SNR %d", data->State == 0 ? "Success" : "Failed",
		  data->DemodMargin, data->NbGateways,
		  data->Rssi, data->Snr);
}

/**
 * @brief LoRaWAN callback after TX is finished
 *
 * @param status TX status
 */
void sendCallback(int32_t status)
{
	MYLOG("TX-CB", "TX status %d", status);
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback if a packet was received
 *
 * @param data pointer to the data with the received data
 */
void recv_cb(rui_lora_p2p_recv_t data)
{
	MYLOG("RX-P2P-CB", "P2P RX, RSSI %d, SNR %d", data.Rssi, data.Snr);
	for (int i = 0; i < data.BufferSize; i++)
	{
		Serial.printf("%02X", data.Buffer[i]);
	}
	Serial.print("\r\n");
	tx_active = false;

	// Check data size
	if (data.BufferSize == 4)
	{
		// Check for valid command sequence
		if ((data.Buffer[0] == 0xAA) && (data.Buffer[1] == 0x55))
		{
			// Check for valid relay status request
			if ((data.Buffer[3] >= 0) && (data.Buffer[3] < 2))
			{
				// Check which relay
				if (data.Buffer[2] == 0) // Water pump relay
				{
					// Save the status and call the handler
					if (data.Buffer[3] == 0)
					{
						relay_pump_status = LOW;
					}
					else
					{
						relay_pump_status = HIGH;
					}
					pump_number = WATER_PUMP;
				}
				else if (data.Buffer[2] == 1) // Nutrition relay
				{
					// Save the status and call the handler
					if (data.Buffer[3] == 0)
					{
						relay_nutrition_status = LOW;
					}
					else
					{
						relay_nutrition_status = HIGH;
					}
					pump_number = NUTRITION_VALVE;
				}
				else if (data.Buffer[2] == 3) // Nutrition refill
				{
					relay_pump_status = LONG;
					pump_number = NUTRITION_VALVE;
				}
				api.system.timer.start(RAK_TIMER_1, 100, (void *)&pump_number);
			}
			else
			{
				MYLOG("RX-P2P-CB", "Wrong command");
			}
		}
		else
		{
			MYLOG("RX-P2P-CB", "Wrong format");
		}
	}
	// Check data size
	else if (data.BufferSize == 11)
	{
		// Check for valid setup sequence
		if ((data.Buffer[0] == 0xAA) && (data.Buffer[1] == 0x55))
		{
			uint32_t new_value_1 = 0;
			uint32_t old_value_1 = 0;
			uint32_t new_value_2 = 0;
			uint32_t old_value_2 = 0;
			// Check for setup type
			switch (data.Buffer[2])
			{
			case REM_TIME:
				old_value_1 = custom_parameters.pump_on_time;
				new_value_1 = data.Buffer[3] << 24;
				new_value_1 |= data.Buffer[4] << 16;
				new_value_1 |= data.Buffer[5] << 8;
				new_value_1 |= data.Buffer[6] << 0;
				MYLOG("RX_CB", "New ON time %ld", new_value_1);
				// custom_parameters.pump_on_time = new_value_1 * 1000;
				old_value_2 = custom_parameters.pump_off_time;
				new_value_2 = data.Buffer[7] << 24;
				new_value_2 |= data.Buffer[8] << 16;
				new_value_2 |= data.Buffer[9] << 8;
				new_value_2 |= data.Buffer[10] << 0;
				MYLOG("RX_CB", "New OFF time %ld", new_value_2);
				custom_parameters.pump_off_time = new_value_2 * 1000;
				// Save custom settings if needed
				if ((old_value_1 != custom_parameters.pump_on_time) || (old_value_2 != custom_parameters.pump_off_time))
				{
					save_at_setting();
				}
				// Stop the timer
				api.system.timer.stop(RAK_TIMER_3);
				// Restart the timer if values != 0
				if ((custom_parameters.pump_on_time != 0) && (custom_parameters.pump_off_time != 0))
				{
					relay_pump_status = LOW;
					digitalWrite(RELAY_PUMP, HIGH);
					// Restart the timer
					api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_on_time, (void *)&relay_pump_status);
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
		MYLOG("RX-P2P-CB", "Wrong size");
	}
}

/**
 * @brief LoRa P2P callback if a packet was sent
 *
 */
void send_cb(void)
{
	MYLOG("TX-P2P-CB", "P2P TX finished");
	digitalWrite(LED_BLUE, LOW);
	tx_active = false;
}

/**
 * @brief LoRa P2P callback for CAD result
 *
 * @param result true if activity was detected, false if no activity was detected
 */
void cad_cb(bool result)
{
	MYLOG("CAD-P2P-CB", "P2P CAD reports %s", result ? "activity" : "no activity");
}

void cb_ble_disconnect(void)
{
	api.system.timer.start(RAK_TIMER_4, 500, NULL);
}

void ble_restart_adv(void *)
{
	MYLOG("BLE-DIS-CB", "Restart advertising");
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.advertise.start(0);
}

/**
 * @brief Arduino setup, called once after reboot/power-up
 *
 */
void setup()
{
	// Setup for LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		g_confirmed_mode = api.lorawan.cfm.get();

		g_confirmed_retry = api.lorawan.rety.get();

		g_data_rate = api.lorawan.dr.get();

		// Setup the callbacks for joined and send finished
		api.lorawan.registerRecvCallback(receiveCallback);
		api.lorawan.registerSendCallback(sendCallback);
		api.lorawan.registerJoinCallback(joinCallback);
		api.lorawan.registerLinkCheckCallback(linkcheckCallback);

		// This application requires Class C to receive data at any time
		api.lorawan.deviceClass.set(2);
	}
	else // Setup for LoRa P2P
	{
		api.lora.registerPRecvCallback(recv_cb);
		api.lora.registerPSendCallback(send_cb);
		api.lora.registerPSendCADCallback(cad_cb);
	}

	pinMode(LED_GREEN, OUTPUT);
	digitalWrite(LED_GREEN, HIGH);
	pinMode(LED_BLUE, OUTPUT);
	digitalWrite(LED_BLUE, HIGH);

	pinMode(WB_IO2, OUTPUT);
	digitalWrite(WB_IO2, HIGH);

	// Start Serial
	Serial.begin(115200);

	// Delay for 5 seconds to give the chance for AT+BOOT
	delay(5000);

	api.system.firmwareVersion.set("RUI3-Relay-V1.0.0");

	Serial.println("RAKwireless RUI3 Node");
	Serial.println("------------------------------------------------------");
	Serial.println("Setup the device with WisToolBox or AT commands before using it");
	Serial.printf("Version %s\n", api.system.firmwareVersion.get().c_str());
	Serial.println("------------------------------------------------------");

	// Initialize module
	Wire.begin();

	// Register the custom AT command to get device status
	if (!init_status_at())
	{
		MYLOG("SETUP", "Add custom AT command STATUS fail");
	}

	// Register the custom AT command to set the send interval
	if (!init_interval_at())
	{
		MYLOG("SETUP", "Add custom AT command Send Interval fail");
	}

	// Register the custom AT command to set the pump on/off times
	if (!init_pump_at())
	{
		MYLOG("SETUP", "Add custom AT command Pump time fail");
	}

	// Get saved sending interval from flash
	get_at_setting();

	digitalWrite(LED_GREEN, LOW);

	// Initialize relay control port
	pinMode(RELAY_PUMP, OUTPUT);
	digitalWrite(RELAY_PUMP, LOW);
	delay(500);
	digitalWrite(RELAY_PUMP, HIGH);
	delay(500);
	digitalWrite(RELAY_PUMP, LOW);
	pinMode(RELAY_NUTRITION, OUTPUT);
	digitalWrite(RELAY_NUTRITION, LOW);
	delay(500);
	digitalWrite(RELAY_NUTRITION, HIGH);
	delay(500);
	digitalWrite(RELAY_NUTRITION, LOW);

	// Create a timer.
	api.system.timer.create(RAK_TIMER_0, send_handler, RAK_TIMER_PERIODIC);
	if (custom_parameters.send_interval != 0)
	{
		// Start a timer.
		api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
	}

	// Create a timer to handle incoming packets
	api.system.timer.create(RAK_TIMER_1, relay_handler, RAK_TIMER_ONESHOT);

	// Create a timer to handle nutrition valve
	api.system.timer.create(RAK_TIMER_2, nutrition_valve_handler, RAK_TIMER_ONESHOT);

	// Create a timer to handle water pump
	api.system.timer.create(RAK_TIMER_3, water_pump_handler, RAK_TIMER_ONESHOT);
	if ((custom_parameters.pump_on_time != 0) && (custom_parameters.pump_off_time != 0))
	{
		MYLOG("SETUP", "Initial start of the pump");
		relay_pump_status = HIGH;
		digitalWrite(RELAY_PUMP, HIGH);
		api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_on_time, (void *)&relay_pump_status);

		send_handler(NULL);
	}

	// Create a timer to handle nutrition valve
	api.system.timer.create(RAK_TIMER_4, ble_restart_adv, RAK_TIMER_ONESHOT);

	// Check if it is LoRa P2P
	if (api.lorawan.nwm.get() == 0)
	{
		digitalWrite(LED_BLUE, LOW);

		// Force continous receive mode
		api.lora.precv(65533);
	}

	if (api.lorawan.nwm.get() == 1)
	{
		if (g_confirmed_mode)
		{
			MYLOG("SETUP", "Confirmed enabled");
		}
		else
		{
			MYLOG("SETUP", "Confirmed disabled");
		}

		MYLOG("SETUP", "Retry = %d", g_confirmed_retry);

		MYLOG("SETUP", "DR = %d", g_data_rate);
	}

	// Enable low power mode
	api.system.lpm.set(1);

#if defined(_VARIANT_RAK3172_) || defined(_VARIANT_RAK3172_SIP_)
// No BLE
#else
	char ble_name[128];
	uint8_t dev_eui[8];
	api.lorawan.deui.get(dev_eui, (uint32_t)8);
	int len = sprintf(ble_name, "RAK-HYDRO-RELAY-%02X%02X%02X", dev_eui[5], dev_eui[6], dev_eui[7]);
	Serial6.begin(115200, RAK_AT_MODE);
	api.ble.settings.broadcastName.set(ble_name, len);
	api.ble.registerCallback(BLE_DISCONNECTED, cb_ble_disconnect);
	api.ble.advertise.start(0);
#endif
}

/**
 * @brief Set or reset the relay, depending on last received packet
 *
 */
void relay_handler(void *number)
{
	uint8_t *pump_number = (uint8_t *)number;
	MYLOG("RELAY", "Set relay %d", pump_number[0]);
	if (pump_number[0] == WATER_PUMP)
	{
		MYLOG("RELAY", "Handle water valve relay override");
		// Handle water pump control
		if (relay_pump_status == HIGH)
		{
			// Manual switch on the water pump start, disable timer
			api.system.timer.stop(RAK_TIMER_3);

			// Change report time to 1 minute
			api.system.timer.stop(RAK_TIMER_0);
			api.system.timer.start(RAK_TIMER_0, 60000, NULL);

			MYLOG("RELAY", "Start water valve relay");
			digitalWrite(RELAY_PUMP, HIGH);
		}
		else if (relay_pump_status == LOW)
		{
			MYLOG("RELAY", "Stop water valve relay");
			digitalWrite(RELAY_PUMP, LOW);
			// Manual switch off the water pump end, enable timer
			if (custom_parameters.pump_on_time != 0)
			{
				relay_pump_status = LOW;
				// digitalWrite(RELAY_PUMP, HIGH);
				api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_off_time, (void *)&relay_pump_status);

				// Restart report time with original setting
				api.system.timer.stop(RAK_TIMER_0);
				api.system.timer.start(RAK_TIMER_0, custom_parameters.send_interval, NULL);
			}
		}
		send_handler(NULL);
	}
	else if (pump_number[0] == NUTRITION_VALVE)
	{
		MYLOG("RELAY", "Handle nutrition valve relay");
		// Handle nutrition valve control
		if (relay_nutrition_status == HIGH)
		{
			MYLOG("RELAY", "Start nutrition valve relay");
			digitalWrite(RELAY_NUTRITION, HIGH);
			api.system.timer.start(RAK_TIMER_2, 10000, NULL);
			relay_nutrition_status = LOW;
			if (relay_pump_status != HIGH)
			{
				// Start Water pump to increase mix of water and nutrition
				digitalWrite(RELAY_PUMP, HIGH);
			}
		}
		else if (relay_nutrition_status == LONG)
		{
			MYLOG("RELAY", "Start nutrition valve relay after refill");
			digitalWrite(RELAY_NUTRITION, HIGH);
			api.system.timer.start(RAK_TIMER_2, 30000, NULL);
			relay_nutrition_status = LOW;
		}
	}
}

/**
 * @brief Stop nutrition valve
 *
 */
void nutrition_valve_handler(void *)
{
	MYLOG("RELAY", "Stop nutrition valve relay");
	digitalWrite(RELAY_NUTRITION, LOW);
	if (relay_pump_status != HIGH)
	{
		MYLOG("RELAY", "Stop water pump as well");
		// Stop Water pump to increase mix of water and nutrition
		digitalWrite(RELAY_PUMP, LOW);
	}
}

/**
 * @brief Handle water pump
 *
 */
void water_pump_handler(void *status)
{
	MYLOG("RELAY", "Handle water pump relay");
	uint8_t *switch_status = (uint8_t *)status;
	if (switch_status[0] == LOW)
	{
		MYLOG("RELAY", "Water pump on");
		digitalWrite(RELAY_PUMP, HIGH);
		relay_pump_status = HIGH;
		api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_on_time, (void *)&relay_pump_status);
	}
	else if (switch_status[0] == HIGH)
	{
		MYLOG("RELAY", "Water pump off");
		digitalWrite(RELAY_PUMP, LOW);
		relay_pump_status = LOW;
		api.system.timer.start(RAK_TIMER_3, custom_parameters.pump_off_time, (void *)&relay_pump_status);
	}
	send_handler(NULL);
}

/**
 * @brief send_handler is a timer function called every
 * g_send_repeat_time milliseconds.
 *
 */
void send_handler(void *)
{
	MYLOG("UPLINK", "Start");
	digitalWrite(LED_BLUE, HIGH);

	if (api.lorawan.nwm.get() == 1)
	{
		// Check if the node has joined the network
		if (!api.lorawan.njs.get())
		{
			MYLOG("UPLINK", "Not joined, skip sending");
			return;
		}
	}

	// Clear payload
	g_solution_data.reset();

	// Create payload
	// Add battery voltage
	g_solution_data.addVoltage(LPP_CHANNEL_BATT, api.system.bat.get());

	MYLOG("UPLINK", "Relay pump status %d readback %d", relay_pump_status, digitalRead(RELAY_PUMP));
	uint32_t pump_is_on = relay_pump_status == LOW ? 0x00 : 0x01;
	MYLOG("UPLINK", "pump_is_on = %d ", pump_is_on);
	g_solution_data.addPresence(LPP_CHANNEL_WATER, pump_is_on);

	// Send the packet
	send_packet();
}

/**
 * @brief This example is complete timer
 * driven. The loop() does nothing than
 * sleep.
 *
 */
void loop()
{
	api.system.sleep.all();
	// api.system.scheduler.task.destroy();
}

/**
 * @brief Send the data packet that was prepared in
 * Cayenne LPP format by the different sensor and location
 * aqcuision functions
 *
 */
void send_packet(void)
{
	// Check if it is LoRaWAN
	if (api.lorawan.nwm.get() == 1)
	{
		MYLOG("UPLINK", "DR was %d", api.lorawan.dr.get());
		api.lorawan.dr.set(get_min_dr(api.lorawan.band.get(), g_solution_data.getSize()));
		MYLOG("UPLINK", "DR used %d", api.lorawan.dr.get());
		// Send the packet
		if (api.lorawan.send(g_solution_data.getSize(), g_solution_data.getBuffer(), set_fPort, g_confirmed_mode, g_confirmed_retry))
		{
			MYLOG("UPLINK", "Packet enqueued, size %d", g_solution_data.getSize());
			tx_active = true;
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
			tx_active = false;
		}
	}
	// It is P2P
	else
	{
		MYLOG("UPLINK", "Send packet over P2P");

		digitalWrite(LED_BLUE, LOW);

		if (api.lora.psend(g_solution_data.getSize(), g_solution_data.getBuffer(), true))
		{
			MYLOG("UPLINK", "Packet enqueued");
		}
		else
		{
			MYLOG("UPLINK", "Send failed");
		}
	}
}
