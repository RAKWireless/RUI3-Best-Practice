/**
 * @file RAK1906.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Bosch BME680 with IAQ algo
 * @version 0.1
 * @date 2024-07-15
 *
 * @copyright Copyright (c) 2024
 *
 */

/*******************************************************************************************************************/
/** License for the BME680 IAQ algorithm                                                                           */
/** Origin from https://github.com/G6EJD/BME680-Example                                                            */
/*******************************************************************************************************************/
/*
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.

 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.

 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
*/

#include "app.h"

#include <Wire.h>
#include "src/rak1906.h"

rak1906 bme;

bool has_rak1906 = false;

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int getgasreference_count = 0;
float gas_lower_limit = 5000;  // Bad air quality limit
float gas_upper_limit = 50000; // Good air quality limit

float volatile g_last_temperature = 0.0;
float volatile g_last_humidity = 0.0;
float volatile g_last_pressure = 0.0;
float volatile g_last_gas = 0.0;
float volatile g_last_air_quality = 0.0;
float volatile g_last_iaq = 0.0;
/**
 * @brief Initialize BME680
 *
 * @return true sensor found
 * @return false no sensor
 */
bool init_rak1906(void)
{
	Wire.begin();
	if (!bme.init())
	{
		MYLOG("BME", "Could not find a valid BME680 sensor, check wiring!");
		return false;
	}

	// Set up oversampling and filter initialization
	bme.setOversampling(TemperatureSensor, Oversample8);
	bme.setOversampling(HumiditySensor, Oversample2);
	bme.setOversampling(PressureSensor, Oversample4);
	bme.setIIRFilter(IIR4);
	bme.setGas(320, 150); // 320°C for 150 ms
	// Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
	GetGasReference();
	return true;
}

/**
 * @brief Read data from BME680
 *
 */
void read_bme680(void *)
{
	if (!bme.update())
	{
		MYLOG("BME", "BME reading timeout");
		return;
	}
	g_last_temperature = bme.temperature();
	g_last_humidity = bme.humidity();
	g_last_pressure = bme.pressure();
	g_last_gas = bme.gas() * 1000.0;

	if (g_last_humidity >= 38 && g_last_humidity <= 42)
		hum_score = 0.25 * 100; // Humidity +/-5% around optimum
	else
	{
		// sub-optimal
		if (g_last_humidity < 38)
			hum_score = 0.25 / hum_reference * g_last_humidity * 100;
		else
		{
			hum_score = ((-0.25 / (100 - hum_reference) * g_last_humidity) + 0.416666) * 100;
		}
	}

	GetGasReference();

	if (gas_reference > gas_upper_limit)
		gas_reference = gas_upper_limit;
	if (gas_reference < gas_lower_limit)
		gas_reference = gas_lower_limit;
	gas_score = (0.75 / (gas_upper_limit - gas_lower_limit) * gas_reference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100;

	// MYLOG("BME", "G %.2f H %.2f", gas_score, hum_score);
	g_last_air_quality = hum_score + gas_score;
	g_last_iaq = (100 - g_last_air_quality) * 5.0;

	MYLOG("BME", "T = %.2f", g_last_temperature);
	MYLOG("BME", "H = %.2f", g_last_humidity);
	MYLOG("BME", "P = %.2f", g_last_pressure);
	MYLOG("BME", "Gas = %.2f", g_last_gas);
	MYLOG("BME", "IAQ score = %.2f", g_last_iaq);
}

/**
 * @brief Update gas reference
 *
 */
void GetGasReference()
{
	// Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
	// MYLOG("BME", "Getting a new gas reference value");
	int readings = 10;
	// int readings = 3;
	for (int i = 1; i <= readings; i++)
	{
		// read gas for 10 x 0.150mS = 1.5secs
		gas_reference += (bme.gas() * 1000.0);
	}
	gas_reference = gas_reference / readings;
}
