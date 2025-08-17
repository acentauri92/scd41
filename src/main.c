#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "scd41.h"
#include "rpi_i2c_hal.h"
#include "influx_logger.h"

#define NUM_OF_SAMPLES_REQUIRED     10

int main() {

    int8_t result;
    uint16_t valid_samples_collected = 0;

    // Initialize the hardware
    if (rpi_i2c_hal_init() != 0) {
        printf("HAL initialization failed. Exiting.\n");
        return 1;
    }

    if (influx_logger_init() != 0) {
        printf("Influx logger initialization failed. Exiting.\n");
        rpi_i2c_hal_close();
        return 1;
    }

    // Stop any unintended measurements
    result = scd41_stop_periodic_measurement();
    if(result != SCD41_OK) {
        printf("Stop periodic measurement failed with error %d\n", result);
        rpi_i2c_hal_close();
        return -1;
    }

    printf("Triggering periodic measurement...\n");
    scd41_measurement_t measurement[NUM_OF_SAMPLES_REQUIRED] = {0};

    result = scd41_start_periodic_measurement();

    if (result == SCD41_OK) {

        while(valid_samples_collected < NUM_OF_SAMPLES_REQUIRED) {
            sleep(5);
            bool is_data_ready = false;

            if (scd41_get_data_ready_status(&is_data_ready) != SCD41_OK) {
                printf("Warning: Failed to get data ready status. Retrying...\n");
                continue;
            }

            if(is_data_ready) {
                result = scd41_read_measurement(&measurement[valid_samples_collected]);
                if(result != SCD41_OK){
                    printf("Warning: Failed to get valid measurement (Error: %d). \
                            Discarding and retrying...\n", result);
                    continue;
                }

                // Data is valid at this point. Print and increment sample count
                printf("Sample %d -> CO2: %d ppm, Temp: %.2f C, RH: %.2f %%\n",
                        valid_samples_collected + 1,
                        measurement[valid_samples_collected].co2_ppm,
                        measurement[valid_samples_collected].temperature_c,
                        measurement[valid_samples_collected].humidity_rh);

                valid_samples_collected++;
            }

            else {
                printf("Warning: Data was not ready in time. Retrying...\n");
            }
        }
        
        // Stop measurement before exiting.
        printf("Stopping periodic measurements...\n");
        result = scd41_stop_periodic_measurement();
        if(result != SCD41_OK)
            return result;
    } 

    else {
        printf("Error: Failed to start periodic measurement. (Error code: %d)\n", result);
    }

    // Calculate average
    float total_co2 = 0;
    float total_temp = 0;
    float total_rh = 0;

    for(int i = 0; i < NUM_OF_SAMPLES_REQUIRED; i++) {   
        total_co2 = total_co2 + measurement[i].co2_ppm;
        total_temp = total_temp + measurement[i].temperature_c;
        total_rh = total_rh + measurement[i].humidity_rh;
    }

    float avg_co2 = total_co2 / NUM_OF_SAMPLES_REQUIRED;
    float avg_temp = total_temp / NUM_OF_SAMPLES_REQUIRED;
    float avg_rh = total_rh / NUM_OF_SAMPLES_REQUIRED;

    printf("Average CO2: %.2f \n", avg_co2);
    printf("Average Temperature: %.2f \n", avg_temp);
    printf("Average RH: %.2f \n", avg_rh);

    scd41_measurement_t avg_measurement = {0};
    avg_measurement.co2_ppm = avg_co2;
    avg_measurement.humidity_rh = avg_rh;
    avg_measurement.temperature_c = avg_temp;

    influx_logger_send_scd41_data(&avg_measurement);

    rpi_i2c_hal_close();

    return 0;
}
