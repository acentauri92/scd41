#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include "scd41.h"
#include "rpi_i2c_hal.h"


/**
 * @brief Helper function to check if a file exists.
 * @param filename The name of the file to check.
 * @return True if the file exists, false otherwise.
 */
bool file_exists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

int main() {

    const char* log_filename = "sensor_log.csv";
    bool needs_header = !file_exists(log_filename);

    // Open the log file for appending data.
    FILE *log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return 1;
    }

    // If the file is new, write the CSV header.
    if (needs_header) {
        fprintf(log_file, "Timestamp,Avg_CO2_ppm,Avg_Temp_C,Avg_Humidity_RH\n");
    }

    // Initialize the hardware
    if (rpi_i2c_hal_init() != 0) {
        printf("HAL initialization failed. Exiting.\n");
        return 1;
    }

    // --- Perform Single Shot Measurement ---
    printf("Triggering periodic measurement...\n");
    scd41_measurement_t measurement[10];
    int8_t result = scd41_start_periodic_measurement();

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

    // Get current time for the log entry
    time_t now = time(NULL);
    char time_buffer[32];
    // Format the time into an ISO 8601 string (e.g., "2023-10-27T10:30:00")
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%dT%H:%M:%S", localtime(&now));

    
    // Write the averages as a new line in the CSV file
    fprintf(log_file, "%s,%.2f,%.2f,%.2f\n", time_buffer, avg_co2, avg_temp, avg_rh);
    printf("Averages written to %s\n", log_filename);
    
    fclose(log_file);
    rpi_i2c_hal_close();

    return 0;
}
