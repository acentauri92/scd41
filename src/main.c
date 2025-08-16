#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "scd41.h"
#include "rpi_i2c_hal.h"
#include "influx_logger.h"

int main() {

    int8_t result;

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
    scd41_measurement_t measurement[10];

    result = scd41_start_periodic_measurement();

    if (result == SCD41_OK) {
        uint8_t i  = 10;

        while ( i-- > 0) {
            sleep(5);
            bool is_data_ready = false;
            int8_t result = scd41_get_data_ready_status(&is_data_ready);
            if(result != SCD41_OK)
                return result;

            if(is_data_ready) {
                result = scd41_read_measurement(&measurement[i]);
                if(result != SCD41_OK)
                    return result;

                printf("  CO2: %d ppm\n", measurement[i].co2_ppm);
                printf("  Temperature: %.2f C\n", measurement[i].temperature_c);
                printf("  Humidity: %.2f %%RH\n\n", measurement[i].humidity_rh);
            }
        }
        
        // Stop measurement before exiting.
        printf("Stopping periodic measurements...\n");
        result = scd41_stop_periodic_measurement();
        if(result != SCD41_OK)
            return result;
    } 

    else {
        printf("Error: Failed to perform periodic measurement. (Error code: %d)\n", result);
    }

    // Calculate average
    float total_co2 = 0;
    float total_temp = 0;
    float total_rh = 0;

    for(int i = 0; i < 10; i++) {   
        total_co2 = total_co2 + measurement[i].co2_ppm;
        total_temp = total_temp + measurement[i].temperature_c;
        total_rh = total_rh + measurement[i].humidity_rh;
    }

    float avg_co2 = total_co2 / 10;
    float avg_temp = total_temp / 10;
    float avg_rh = total_rh / 10;

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
