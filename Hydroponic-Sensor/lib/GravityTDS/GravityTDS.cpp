/***************************************************
 DFRobot Gravity: Analog TDS Sensor/Meter
 <https://www.dfrobot.com/wiki/index.php/Gravity:_Analog_TDS_Sensor_/_Meter_For_Arduino_SKU:_SEN0244>
 
 ***************************************************
 This sample code shows how to read the tds value and calibrate it with the standard buffer solution.
 707ppm(1413us/cm)@25^c standard buffer solution is recommended.
 
 Created 2018-1-3
 By Jason <jason.ling@dfrobot.com@dfrobot.com>
 
 GNU Lesser General Public License.
 See <http://www.gnu.org/licenses/> for details.
 All above must be included in any redistribution.
 ****************************************************/

#include "GravityTDS.h"

GravityTDS::GravityTDS()
{
    this->pin = A1;
    this->temperature = 25.0;
    this->aref = 5.0;
    this->adcRange = 1024.0;
    this->kValueAddress = 8;
    this->kValue = 1.0;
}

GravityTDS::~GravityTDS()
{
}

void GravityTDS::setPin(int pin)
{
	this->pin = pin;
}

void GravityTDS::setTemperature(float temp)
{
	this->temperature = temp;
}

void GravityTDS::setAref(float value)
{
	this->aref = value;
}

void GravityTDS::setAdcRange(float range)
{
      this->adcRange = range;
}

void GravityTDS::setKvalueAddress(int address)
{
      this->kValueAddress = address;
}

void GravityTDS::begin()
{
	pinMode(this->pin,INPUT);
}

float GravityTDS::getKvalue()
{
	return this->kValue;
}

void GravityTDS::update()
{
	this->analogValue = analogRead(this->pin);
	this->voltage = this->analogValue/this->adcRange*this->aref;
	this->ecValue=(133.42*this->voltage*this->voltage*this->voltage - 255.86*this->voltage*this->voltage + 857.39*this->voltage)*this->kValue;
	this->ecValue25  =  this->ecValue / (1.0+0.02*(this->temperature-25.0));  //temperature compensation
	this->tdsValue = ecValue25 * TdsFactor;
}

float GravityTDS::getTdsValue()
{
	return tdsValue;
}

float GravityTDS::getEcValue()
{
      return ecValue25;
}
