#include <stdio.h>
#include <unistd.h>
#include "scd41.h"
#include "rpi_i2c_hal.h"

int main() {
    printf("--- SCD41 Single Shot Measurement Test ---\n");

    // Initialize the hardware
    if (rpi_i2c_hal_init() != 0) {
        printf("HAL initialization failed. Exiting.\n");
        return 1;
    }

    // --- Robust Startup ---
    // Wake the sensor up from its low-power idle state.
    printf("Waking sensor...\n");
    if (scd41_wakeup() != 0) {
        printf("Error: Could not wake up sensor. Trying to continue...\n");
    } else {
        printf("Sensor is awake.\n");
    }
    // Give it a moment to stabilize after waking up.
    sleep(1);


    // --- Perform Single Shot Measurement ---
    printf("Triggering single shot measurement. This will take 5 seconds...\n");
    scd41_measurement_t measurement;
    int8_t result = scd41_measure_single_shot(&measurement);

    if (result == 0) {
        printf("Measurement successful!\n");
        printf("  CO2: %d ppm\n", measurement.co2_ppm);
        printf("  Temperature: %.2f C\n", measurement.temperature_c);
        printf("  Humidity: %.2f %%RH\n", measurement.humidity_rh);
    } else {
        printf("Error: Failed to perform single shot measurement. (Error code: %d)\n", result);
    }

    // Clean up
    rpi_i2c_hal_close();
    return 0;
}
