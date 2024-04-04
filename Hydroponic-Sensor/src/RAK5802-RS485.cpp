/**
 * @file RAK5802-RS485.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Initialization and MODbus usage for RAK5802
 * @version 0.1
 * @date 2024-03-28
 *
 * @copyright Copyright (c) 2024
 *
 */
#include "main.h"
#include "RUI3_ModbusRtu.h"

/** Data array for modbus  */
int16_t coils_n_regs[2];

/**
 *  Modbus object declaration
 *  u8id : node id = 0 for master, = 1..247 for slave
 *  port : serial port
 *  u8txenpin : 0 for RS-232 and USB-FTDI
 *               or any pin number > 1 for RS-485
 */
Modbus master(0, Serial1, 0); // this is master and RS-232 or USB-FTDI

/** This is an structure which contains a query to an slave device */
modbus_t telegram;

void init_rak5802(void)
{
	Serial1.begin(9600); // baud-rate at 9600
	master.start();
	master.setTimeOut(2000); // if there is no answer in 2000 ms, roll over
}

bool read_rak5802(uint8_t sensor_type)
{
	uint32_t coll_data = 0;
	uint8_t valid_readings = 0;
	uint8_t sensor_addr = 1;

	if (sensor_type == LPP_PH)
	{
		sensor_addr = 1;
	}
	else if (sensor_type == LPP_TDS)
	{
		sensor_addr = 2;
	}

	for (int i = 0; i < 5; i++)
	{
		MYLOG("MODR", "Send read request %d over ModBus", i + 1);

		coils_n_regs[0] = coils_n_regs[1] = 0;

		telegram.u8id = sensor_addr;		   // slave address
		telegram.u8fct = MB_FC_READ_REGISTERS; // function code (this one is registers read)
		telegram.u16RegAdd = 0x0d;			   // start address in slave
		telegram.u16CoilsNo = 1;			   // number of elements (coils or registers) to read
		telegram.au16reg = coils_n_regs;	   // pointer to a memory array in the Arduino

		master.query(telegram); // send query (only once)

		time_t start_poll = millis();

		while ((millis() - start_poll) < 5000)
		{
			master.poll(); // check incoming messages
			if (master.getState() == COM_IDLE)
			{
				MYLOG("MODR", "pH %04X %.2f", coils_n_regs[0], (float)(coils_n_regs[0]) / 10.0);
				if (coils_n_regs[0] == 0)
				{
					MYLOG("MODR", "No data received");
					break;
				}
				else
				{
					MYLOG("MODR", "pH %.2f", (float)(coils_n_regs[0]) / 10.0);
					coll_data += coils_n_regs[0];
					valid_readings++;
					break;
				}
			}
		}
		delay(2000);
	}

	if (valid_readings != 0)
	{
		coll_data = coll_data / valid_readings;

		if (sensor_type == LPP_PH)
		{
			MYLOG("MODR", "Final pH %.2f", (float)(coll_data) / 10.0);

			payload.addAnalogInput(LPP_PH, (float)(coll_data) / 10.0);
		}
		else if (sensor_type == LPP_TDS)
		{
			// No data sheet yet for the EC (TDS ???) sensor, assuming some stuff here
			MYLOG("MODR", "Final EC %.2f", (float)(coll_data) * 2.0);

			payload.addConcentration(LPP_TDS, coll_data);
		}
	}
	else
	{
		return false;
	}

	return true;
}