// include the necessary libraries
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define LED_GPIO 2       // GPIO 2 is the built-in LED on the ESP32
#define HEATER_GPIO 12   // GPIO 12 is connected to the relay module
#define ADC1_CHANNEL_0 0 // GPIO 36 is the ADC pin for the thermistor

#define TARGET_TEMP 35

#define TEMP_BUFFER_SIZE 10
#define TEMP_DELTA_BUFFER_SIZE 10
#define TEMP_TARGET_DELTA_BUFFER_SIZE 10
#define AV_TEMP_BUFFER_SIZE 10

float correction_power = 0.5;
float control_cycle = 1;

float temperatureBuffer[TEMP_BUFFER_SIZE];
int currentTempBufferIndex = 0;

float averageTemperatureBuffer[AV_TEMP_BUFFER_SIZE];
int currentAverageTempBufferIndex = 0;

float temperatureDeltaBuffer[TEMP_DELTA_BUFFER_SIZE];
int currentTempDeltaBufferIndex = 0;

float temperatureTargetDeltaBuffer[TEMP_TARGET_DELTA_BUFFER_SIZE];
int currentTempTargetDeltaBufferIndex = 0;

float readTemperature()
{
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);                        // Configure ADC to 12-bit resolution
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11); // Configure attenuation for the pin

    // read the ADC value from the thermistor
    int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
    // printf("ADC Reading: %d\n", adc_reading);

    // convert the ADC value to a voltage
    float voltage = (adc_reading * 3.3) / 4095;
    // printf("Voltage: %f\n", voltage);

    // convert the voltage to a celcius temperature
    float resistance = 10 * voltage / (3.3 - voltage);
    // printf("Resistance: %f\n", resistance);

    float tempKelvin = 1 / (1 / (273.15 + 25) + log(resistance / 10) / 3950.0);
    float tempCelsius = tempKelvin - 273.15;

    return tempCelsius;
}

void updateTempBuffer(float temperature)
{
    // Store the temperature in the buffer
    temperatureBuffer[currentTempBufferIndex] = temperature;
    // Increment the index, wrapping around to the start of the buffer if necessary
    currentTempBufferIndex = (currentTempBufferIndex + 1) % TEMP_BUFFER_SIZE;
}

void updateAverageTempBuffer(float temperature)
{
    // Store the temperature in the buffer
    averageTemperatureBuffer[currentAverageTempBufferIndex] = temperature;
    // Increment the index, wrapping around to the start of the buffer if necessary
    currentAverageTempBufferIndex = (currentAverageTempBufferIndex + 1) % AV_TEMP_BUFFER_SIZE;
}

void updateTempDeltaBuffer(float delta)
{
    // Store the temperature delta in the buffer
    temperatureDeltaBuffer[currentTempDeltaBufferIndex] = delta;
    // Increment the index, wrapping around to the start of the buffer if necessary
    currentTempDeltaBufferIndex = (currentTempDeltaBufferIndex + 1) % TEMP_DELTA_BUFFER_SIZE;
}

void updateTargetTempDeltaBuffer(float delta)
{
    // Store the temperature delta in the buffer
    temperatureTargetDeltaBuffer[currentTempTargetDeltaBufferIndex] = delta;
    // Increment the index, wrapping around to the start of the buffer if necessary
    currentTempTargetDeltaBufferIndex = (currentTempTargetDeltaBufferIndex + 1) % TEMP_DELTA_BUFFER_SIZE;
}

float calculateAverageTemp()
{
    // Calculate the average temperature from the buffer values
    float sum = 0;
    int count = 0;
    for (int i = 0; i < TEMP_BUFFER_SIZE; i++)
    {
        if (temperatureBuffer[i] != 0)
        {
            sum += temperatureBuffer[i];
            count++;
        }
    }
    return sum / count;
}

void app_main()
{
    gpio_pad_select_gpio(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_pad_select_gpio(HEATER_GPIO);
    gpio_set_direction(HEATER_GPIO, GPIO_MODE_OUTPUT);

    // gpio_pad_select_gpio(36);
    // gpio_set_direction(36, GPIO_MODE_INPUT);
    float last_temp = 0;

    while (true)
    {

        // gpio_set_level(LED_GPIO, 1);
        // vTaskDelay(100 / portTICK_PERIOD_MS);
        // gpio_set_level(LED_GPIO, 0);
        // vTaskDelay(100 / portTICK_PERIOD_MS);

        // -------------------------------------------------------- AGGREGATE DATA FOR CONTROLLING

        // Read the temperature
        float temp = readTemperature();
        updateTempBuffer(temp);

        float average_temp = calculateAverageTemp(); // Calculate the average temperature to flatten mini temperature spikes
        updateAverageTempBuffer(average_temp);

        // Calculate delta from the current temperature to the last measured temperature
        float temp_delta = average_temp - last_temp;
        last_temp = average_temp;
        updateTempDeltaBuffer(temp_delta);

        // Calculate Temperature Trend from average temperature delta over 1s
        float one_second_old_last_av_temp = averageTemperatureBuffer[9];
        float one_s_trend = average_temp - one_second_old_last_av_temp;

        // Update the temperature-target delta buffer
        float target_delta = TARGET_TEMP - average_temp;
        updateTargetTempDeltaBuffer(target_delta);
        float one_s_old_target_delta = temperatureTargetDeltaBuffer[9];
        float one_s_target_delta_trend = target_delta - one_s_old_target_delta;


        // -------------------------------------------------------- CONTROL LOGIC
        if (target_delta > 0.5) // action required
        {
        }
        else // NO action required, target within bounds
        {
            continue;
        }

        // -------------------------------------------------------- CONTROL HEATER
        float cycle = control_cycle / 100;
        if (correction_power <= cycle)
        {
            // power on
            gpio_set_level(HEATER_GPIO, 1);
        }
        else
        {
            // power off
            gpio_set_level(HEATER_GPIO, 0);
        }

        printf(" | Temp: %f", temp);
        if(one_s_trend >= 0){
            printf(" | one_s_trend:  %f", one_s_trend);
        }else{
            printf(" | one_s_trend: %f", one_s_trend);
        }

        if(one_s_target_delta_trend >= 0){
            printf(" | one_s_target_delta_trend:  %f", one_s_target_delta_trend);
        }else{
            printf(" | one_s_target_delta_trend: %f", one_s_target_delta_trend);
        }
        printf(" | Target Delta: %f", target_delta);
        // printf(" | Correction power: %f", correction_power);

        printf("\n");

        vTaskDelay(100 / portTICK_PERIOD_MS);

        if (control_cycle == 100)
        {
            control_cycle = 0;
        }
        else
        {
            control_cycle++;
        }
    }
}