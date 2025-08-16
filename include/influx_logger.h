#ifndef INFLUX_LOGGER_H
#define INFLUX_LOGGER_H

#include <stdint.h>
#include "scd41.h"

/**
 * @brief Initializes the InfluxDB logger and tests the connection.
 * @return 0 on success, non-zero on failure.
 */
int influx_logger_init(void);

/**
 * @brief Sends SCD41 sensor data to InfluxDB.
 * @param measurement A pointer to the struct containing the sensor readings.
 */
void influx_logger_send_scd41_data(const scd41_measurement_t* measurement);


/**
 * @brief Cleans up and closes the InfluxDB logger.
 */
void influx_logger_cleanup(void);

#endif // INFLUX_LOGGER_H