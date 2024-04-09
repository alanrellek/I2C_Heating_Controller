// include the necessary libraries
#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/gpio.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define BLINK_GPIO 2
#define POWER_GPIO 12
#define TEMP_GPIO 36
#define ADC1_CHANNEL_0 0

void app_main()
{
    // GPIO 2 is the built-in LED on the ESP32, turn it on and off every second
    gpio_pad_select_gpio(2); // GPIO 2 is the built-in LED on the ESP32
    gpio_set_direction(2, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(12); // GPIO 12 is connected to the relay module
    gpio_set_direction(12, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(36); // GPIO 36 is the ADC pin for the thermistor
    gpio_set_direction(36, GPIO_MODE_INPUT);

    while (1)
    {
        // printf("Turning on\n");
        gpio_set_level(2, 1);
        // gpio_set_level(12, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // printf("Turning off\n");
        gpio_set_level(2, 0);
        // gpio_set_level(12, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // read the ADC value from the thermistor

        int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
        // printf("ADC Reading: %d\n", adc_reading);

        // convert the ADC value to a voltage
        float voltage = (adc_reading * 3.3) / 4095;
        printf("Voltage: %f\n", voltage);

        // convert the voltage to a celcius temperature
        float resistance = 10 * voltage / (3.3 - voltage);
        printf("Resistance: %f\n", resistance);

        float tempKelvin = 1 / (1 / (273.15 + 25) + log(resistance / 10) / 3950.0);
        float tempCelsius = tempKelvin - 273.15;

        printf("Temperature: %f\n", tempCelsius);
    }
}