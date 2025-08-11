#include <stdio.h>
#include <unistd.h>
#include "scd41.h"
#include "rpi_i2c_hal.h"

int main() {
    // Initialize the hardware
    if (rpi_i2c_hal_init() != 0) {
        printf("HAL initialization failed. Exiting.\n");
        return 1;
    }
    int8_t result;

    uint16_t current_altitude = 0;
    uint16_t altitude_to_set = 930;
    uint32_t pressure_to_set = 101400;
    uint32_t current_pressure = 0;


    result = scd41_get_sensor_altitude(&current_altitude);
    if(result != SCD41_OK)
        return result;

    result = scd41_set_ambient_pressure(pressure_to_set);
    if(result != SCD41_OK)
        return result;

    result = scd41_get_ambient_pressure(&current_pressure);
    if(result != SCD41_OK)
        return result;

    printf("Sensor current altitude: %d\n", current_altitude);
    printf("Sensor current pressurer: %d\n", current_pressure);

    rpi_i2c_hal_close();

    return 0;
}

/* Periodic measurement ex*/
// int main() {
//     // Initialize the hardware
//     if (rpi_i2c_hal_init() != 0) {
//         printf("HAL initialization failed. Exiting.\n");
//         return 1;
//     }

//     int8_t result = scd41_stop_periodic_measurement();
//     if(result != SCD41_OK)
//         return result;

//     // --- Perform Single Shot Measurement ---
//     printf("Triggering periodic measurement...\n");
//     scd41_measurement_t measurement;
//     result = scd41_start_periodic_measurement();

//     if (result == SCD41_OK) {
//         uint8_t i  = 10;

//         while ( i-- > 0) {
//             sleep(5);
//             bool is_data_ready = false;
//             int8_t result = scd41_get_data_ready_status(&is_data_ready);
//             if(result != SCD41_OK)
//                 return result;

//             if(is_data_ready) {
//                 result = scd41_read_measurement(&measurement);
//                 if(result != SCD41_OK)
//                     return result;

//                 printf("  CO2: %d ppm\n", measurement.co2_ppm);
//                 printf("  Temperature: %.2f C\n", measurement.temperature_c);
//                 printf("  Humidity: %.2f %%RH\n\n", measurement.humidity_rh);
//             }
//         }
        
//         // Stop measurement before exiting.
//         result = scd41_stop_periodic_measurement();
//         if(result != SCD41_OK)
//             return result;
//     } 

//     else {
//         printf("Error: Failed to perform periodic measurement. (Error code: %d)\n", result);
//     }

//     rpi_i2c_hal_close();

//     return 0;
// }
