#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "lib_sensirion/svm41_i2c.h"


void main(void)
{
	
    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = svm41_device_reset();
    if (error) {
        printf("Error executing svm41_device_reset(): %i\n", error);
    }

    unsigned char serial_number[26];
    uint8_t serial_number_size = 26;
    error = svm41_get_serial_number(serial_number, serial_number_size);
    if (error) {
        printf("Error executing svm41_get_serial_number(): %i\n", error);
    } else {
        printf("Serial number: %s\n", serial_number);
    }

    uint8_t firmware_major;
    uint8_t firmware_minor;
    bool firmware_debug;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    error = svm41_get_version(&firmware_major, &firmware_minor, &firmware_debug,
                              &hardware_major, &hardware_minor, &protocol_major,
                              &protocol_minor);

    if (error) {
        printf("Error executing svm41_get_version(): %i\n", error);
    } else {
        printf("Firmware: %i.%i Debug: %i\n", firmware_major, firmware_minor,
               firmware_debug);
        printf("Hardware: %i.%i\n", hardware_major, hardware_minor);
        printf("Protocol: %i.%i\n", protocol_major, protocol_minor);
    }

    int16_t t_offset;
    error = svm41_get_temperature_offset_for_rht_measurements(&t_offset);
    if (error) {
        printf("Error executing "
               "svm41_get_temperature_offset_for_rht_measurements(): %i\n",
               error);
    } else {
        printf("Temperature Offset: %i ticks\n", t_offset);
    }

    // Start Measurement
    error = svm41_start_measurement();
    if (error) {
        printf("Error executing svm41_start_measurement(): %i\n", error);
    }

    for (;;) {
        // Read Measurement
        sensirion_i2c_hal_sleep_usec(10000000); // Delay 10s
        int16_t humidity;
        int16_t temperature;
        int16_t voc_index;
        int16_t nox_index;
        error = svm41_read_measured_values_as_integers(&humidity, &temperature,
                                                       &voc_index, &nox_index);
        if (error) {
            printf("Error executing svm41_read_measured_values_as_integers(): "
                   "%i\n",
                   error);
        } else {
            printf("Humidity: %i milli %% RH\n", humidity * 10);
            printf("Temperature: %i milli °C\n", (temperature >> 1) * 10);
            printf("VOC index: %i (index * 10)\n", voc_index);
            printf("NOx index: %i (index * 10)\n", nox_index);
        }
    }

    error = svm41_stop_measurement();

    if (error) {
        printf("Error executing svm41_stop_measurement(): %i\n", error);
    }

    return 0;

}