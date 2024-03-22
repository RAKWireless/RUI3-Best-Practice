/**
 * @file RAK12047_voc.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief Read values from the RAK12047 VOC sensor
 * @version 0.1
 * @date 2022-04-11
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "app.h"
#include <SensirionI2CSgp40.h>
#include <VOCGasIndexAlgorithm.h>

/** Sampling interval for the algorithm in seconds */
int32_t sampling_interval = 30;
/** Number of samples to be discarded */
int32_t discard_sampling = 50;

/** Instance for the VOC sensor */
SensirionI2CSgp40 sgp40;
/** Instance for the VOC algorithm */
VOCGasIndexAlgorithm voc_algorithm(sampling_interval);

/** Calculated VOC index */
int32_t voc_index = 0;

/** Flag if the VOC index is valid */
bool voc_valid = false;

/** Buffer for debug output */
char errorMessage[256];

/** Counter to discard the first 100 readings */
uint16_t discard_counter = 0;

volatile bool sensor_reading_active = false;

/** Flag if values were read already (set by RAK1901 or RAK1906) */
bool g_has_last_values = false;
/** Last temperature read */
float g_last_temp = 0;
/** Last humidity read */
float g_last_humid = 0;
/** Counter to discard saved values after some time */
uint8_t t_h_discard_counter = 0;

// Forward declaration
void do_read_rak12047(void *);

/**
 * @brief Initialize the sensor
 *
 * @return true success
 * @return false failed
 */
bool init_rak12047(void)
{
	sgp40.begin(Wire);

	uint16_t serialNumber[3];
	uint8_t serialNumberSize = 3;

	uint16_t error = sgp40.getSerialNumber(serialNumber, serialNumberSize);

	if (error)
	{
		// errorToString(error, errorMessage, 256);
		// MYLOG("VOC", "Error trying to execute getSerialNumber() %s", errorMessage);
		return false;
	}
	else
	{
		// #if MY_DEBUG > 0
		// 		Serial.print("SerialNumber:");
		// 		Serial.print("0x");
		// 		for (size_t i = 0; i < serialNumberSize; i++)
		// 		{
		// 			uint16_t value = serialNumber[i];
		// 			Serial.print(value < 4096 ? "0" : "");
		// 			Serial.print(value < 256 ? "0" : "");
		// 			Serial.print(value < 16 ? "0" : "");
		// 			Serial.print(value, HEX);
		// 		}
		// 		Serial.println();
		// #endif
	}

	uint16_t testResult;
	error = sgp40.executeSelfTest(testResult);
	if (error)
	{
		// errorToString(error, errorMessage, 256);
		// MYLOG("VOC", "Error trying to execute executeSelfTest() %s", errorMessage);
		return false;
	}
	else if (testResult != 0xD400)
	{
		// MYLOG("VOC", "executeSelfTest failed with error %d", testResult);
		return false;
	}

	int32_t index_offset;
	int32_t learning_time_offset_hours;
	int32_t learning_time_gain_hours;
	int32_t gating_max_duration_minutes;
	int32_t std_initial;
	int32_t gain_factor;
	voc_algorithm.get_tuning_parameters(
		index_offset, learning_time_offset_hours, learning_time_gain_hours,
		gating_max_duration_minutes, std_initial, gain_factor);

	// Reset discard counter
	discard_counter = 0;

	// Set VOC reading interval to 10 seconds
	// Create a unified timer
	api.system.timer.create(RAK_TIMER_1, do_read_rak12047, RAK_TIMER_PERIODIC);
	// Start a unified C timer
	api.system.timer.start(RAK_TIMER_1, sampling_interval * 1000, NULL);

	return true;
}

/**
 * @brief Read the last VOC index
 *     Data is added to Cayenne LPP payload as channel
 *     LPP_CHANNEL_VOC
 *
 */
void get_rak12047_value(void)
{
	// MYLOG("VOC", "Get VOC");
	if (voc_valid)
	{
		// MYLOG("VOC", "VOC Index: %ld", voc_index);

		g_solution_data.addVoc_index(LPP_CHANNEL_VOC, voc_index);
	}
	else
	{
		// MYLOG("VOC", "No valid VOC available");
	}
}

/**
 * @brief Read the current VOC and feed it to the
 *        VOC algorithm
 *        Called every 1 second by a timer
 *
 */
void do_read_rak12047(void *)
{
#if MY_DEBUG > 0
	digitalWrite(LED_BLUE, HIGH);
#endif

	// If sensor heatup phase is not active, power up sensor
	if (!sensor_reading_active)
	{
		// Power up sensors
		digitalWrite(WB_IO2, HIGH);
		delay(500);
		Wire.begin();
	}
	uint16_t error;
	uint16_t srawVoc = 0;
	uint16_t defaultRh = 0x8000; // %RH
	uint16_t defaultT = 0x6666;	 // degreeC
	float t_h_values[2] = {0.0};

	if (has_rak1901)
	{
		get_rak1901_values(t_h_values);
		// MYLOG("VOC", "Rh: %.2f T: %.2f", humidity, temperature);

		if ((t_h_values[0] != 0.0) && (t_h_values[1] != 0.0))
		{
			defaultRh = (uint16_t)(t_h_values[0] * 65535 / 100);
			defaultT = (uint16_t)((t_h_values[1] + 45) * 65535 / 175);
		}
	}
	if (has_rak1906)
	{
		get_rak1906_values(t_h_values);
		// MYLOG("VOC", "Rh: %.2f T: %.2f", humidity, temperature);

		if ((t_h_values[0] != 0.0) && (t_h_values[1] != 0.0))
		{
			defaultRh = (uint16_t)(t_h_values[0] * 65535 / 100);
			defaultT = (uint16_t)((t_h_values[1] + 45) * 65535 / 175);
		}
	}
	else if (has_rak12037)
	{
		// Try to get values from RAK12037
		get_rak12037_values(t_h_values);
		// MYLOG("VOC", "Rh: %.2f T: %.2f", humidity, temperature);

		if ((t_h_values[0] != 0.0) && (t_h_values[1] != 0.0))
		{
			defaultRh = (uint16_t)(t_h_values[0] * 65535 / 100);
			defaultT = (uint16_t)((t_h_values[1] + 45) * 65535 / 175);
		}
	}

	// 2. Measure SGP4x signals
	error = sgp40.measureRawSignal(defaultRh, defaultT,
								   srawVoc);
	// MYLOG("VOC", "VOC: %d", srawVoc);

	// 3. Process raw signals by Gas Index Algorithm to get the VOC index values
	if (error)
	{
		errorToString(error, errorMessage, 256);
		MYLOG("VOC", "SGP40 - Error trying to execute measureRawSignals(): %s", errorMessage);
	}
	else
	{
		if (discard_counter <= discard_sampling)
		{
			// Discard the first xxx readings
			voc_algorithm.process(srawVoc);
			discard_counter++;
			MYLOG("VOC", "Discard reading %d", discard_counter);
		}
		else if (discard_counter == discard_sampling + 1)
		{
			// First accepted reading
			voc_index = voc_algorithm.process(srawVoc);
			discard_counter++;
			MYLOG("VOC", "First good reading: %ld", voc_index);
		}
		else
		{
			voc_index = ((voc_index + voc_algorithm.process(srawVoc)) / 2);
			MYLOG("VOC", "VOC Index: %ld", voc_index);
		}
		voc_valid = true;
	}

	// Make sure heater is off
	uint16_t success = 0xFFFF;
	
	for (int retry = 0; retry < 5; retry++)
	{
		success = sgp40.turnHeaterOff();
		if (success == 0)
		{
			MYLOG("VOC", "RAK12047 Heater off %d", retry);
			break;
		}
		delay(100);
	}
	if (success != 0)
	{
		MYLOG("VOC", "RAK12047 Heater off failed");
	}

	// Check if sensor heatup is finished
	if (sensor_heatup_finished)
	{
		sensor_heatup_finished = false;
		read_send_sensors();
	}

	// If no active sensor readings, shut down sensor power
	if (!sensor_reading_active)
	{
		Wire.end();
		// Power down sensors
		digitalWrite(WB_IO2, LOW);
	}
#if MY_DEBUG > 0
	digitalWrite(LED_BLUE, LOW);
#endif
}
