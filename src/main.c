// include the necessary libraries
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define LED_GPIO 2 // GPIO 2 is the built-in LED on the ESP32
#define POWER_GPIO 12 // GPIO 12 is connected to the relay module
#define ADC1_CHANNEL_0 0 // GPIO 36 is the ADC pin for the thermistor

#define UPPER_TEMP 40
#define LOWER_TEMP 39

#define BUFFER_SIZE 100
#define TEMP_DELTA_BUFFER_SIZE 100

float temperatureBuffer[BUFFER_SIZE];
int currentTempBufferIndex = 0;

float temperatureDeltaBuffer[TEMP_DELTA_BUFFER_SIZE];
int currentTempDeltaBufferIndex = 0;

float readTemperature()
{
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

    // printf("Temperature: %f\n", tempCelsius);
    return tempCelsius;
}

void updateTempBuffer(float temperature)
{
    // Store the temperature in the buffer
    temperatureBuffer[currentTempBufferIndex] = temperature;
    // Increment the index, wrapping around to the start of the buffer if necessary
    currentTempBufferIndex = (currentTempBufferIndex + 1) % BUFFER_SIZE;
}

float calculateAverageTemp()
{
    // Calculate the average temperature from the buffer values
    float sum = 0;
    int count = 0;
    for (int i = 0; i < BUFFER_SIZE; i++)
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
    gpio_pad_select_gpio(POWER_GPIO); 
    gpio_set_direction(POWER_GPIO, GPIO_MODE_OUTPUT);
    // gpio_pad_select_gpio(36); 
    // gpio_set_direction(36, GPIO_MODE_INPUT);

    while (true)
    {
        gpio_set_level(LED_GPIO, 1);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 0);
        vTaskDelay(100 / portTICK_PERIOD_MS);


        float temp = readTemperature();

        // Calculate the difference from the current temperature to the last measured temperature
        float diff = 0;
        if (currentTempBufferIndex > 0)
        {
            diff = temp - temperatureBuffer[currentTempBufferIndex - 1];
        }

        // Update the temperature delta buffer
        temperatureDeltaBuffer[currentTempDeltaBufferIndex] = diff;
        currentTempDeltaBufferIndex = (currentTempDeltaBufferIndex + 1) % TEMP_DELTA_BUFFER_SIZE;

        updateTempBuffer(temp);



        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        float averageTemp = calculateAverageTemp();

        // printf("Temp: %f\n", temp);
        printf("Diff: %f\n", diff);
        // printf("Average Temp: %f\n", averageTemp);

    
    }
}