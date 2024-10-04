/**
 * @file ds18b20.cpp
 * @author Bernd Giesecke (bernd@giesecke.tk)
 * @brief DS18B20 read function
 * @version 0.1
 * @date 2024-10-03
 *
 * @copyright ©2024
 *
 * @license: MIT License
 */
#include "app.h"

/**
 * @brief Read temperature from DS18B20 sensor
 *
 * @param ds_temp_1 temperature sensor 1
 * @param ds_temp_2 temperature sensor 1
 * @return true Read success
 * @return false Read failed
 */
bool read_ds18b20(float &ds_temp_1, float &ds_temp_2)
{
    if (has_ds_1)
        ds_1.requestTemp();
    if (has_ds_2)
        ds_2.requestTemp();

    delay(1000);

    bool result = true;
    if (has_ds_1)
        if (ds_1.readTemp())
            ds_temp_1 = ds_1.getTemp();
        else
            result = false;
    if (has_ds_2)
        if (ds_2.readTemp())
            ds_temp_2 = ds_2.getTemp();
        else
            result = false;
    return result;
}