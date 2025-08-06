#ifndef SCD41_H
#define SCD41_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// SCD41 commands
#define SCD41_CMD_GET_SERIAL_NUMBER                     0x3682
#define SCD41_I2C_ADDR                                  0x62
#define SCD41_CMD_READ_MEAS                             0xEC05
#define SCD41_CMD_START_PERIODIC_MEAS                   0x21B1
#define SCD41_CMD_STOP_PERIODIC_MEAS                    0x3F86
#define SCD41_CMD_GET_DATA_READY_STATUS                 0xE4B8
#define SCD41_CMD_REINIT                                0x3646
#define SCD41_CMD_WAKEUP                                0x36F6
#define SCD41_CMD_SINGLE_SHOT_MEAS                      0x219D
#define SCD41_CMD_SET_SENSOR_ALTITUDE                   0x2427
#define SCD41_CMD_GET_SENSOR_ALTITUDE                   0x2322
#define SCD41_CMD_SET_AMBIENT_PRESSURE                  0XE000
#define SCD41_CMD_GET_AMBIENT_PRESSURE                  0XE000


//  Measurement delays in milliseconds (ms) for the sensor.
#define SCD41_START_PERIODIC_MEAS_DELAY_MS              5U
#define SCD41_STOP_PERIODIC_MEAS_DELAY_MS               500U
#define SCD41_READ_MEAS_DELAY_MS                        1U
#define SCD41_GET_DATA_READY_STATUS_DELAY_MS            1U
#define SCD41_REINIT_DELAY_MS                           30U
#define SCD41_WAKEUP_DELAY_MS                           30U
#define SCD41_SINGLE_SHOT_MEAS_DELAY_MS                 5000U
#define SCD41_SET_SENSOR_ALTITUDE_DELAY_MS              1U
#define SCD41_GET_SENSOR_ALTITUDE_DELAY_MS              1U
#define SCD41_SET_AMBIENT_PRESSURE_DELAY_MS             1U
#define SCD41_GET_AMBIENT_PRESSURE_DELAY_MS             1U

/**
 * @brief Error codes
 */
typedef enum {
    SCD41_OK            =  0,
    SCD41_ERR_I2C_WRITE = -1,
    SCD41_ERR_I2C_READ  = -2,
    SCD41_ERR_CRC       = -3,
    SCD41_ERR_WAKEUP    = -4,
} scd41_error_t;

/**
 * @brief Holds a single sensor measurement.
 */
typedef struct {
    uint16_t co2_ppm;
    float temperature_c;
    float humidity_rh;
} scd41_measurement_t;

/**
 * @brief Fills a 2-byte buffer with a 16-bit command in big-endian format.
 * @param command The 16-bit command code (e.g., 0x202f).
 * @param buffer A pointer to a 2-element uint8_t array to be filled.
 */
void scd41_fill_command_buffer(uint16_t command, uint8_t* buffer);

/**
 * @brief Reinitializes the sensor by reloading user settings from EEPROM.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_reinit(void);

/**
 * @brief  Wake up the sensor from sleep mode into idle mode
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_wakeup(void);

/**
 * @brief Reads the unique 48-bit serial number from the sensor.
 * @param serial_number A pointer to a 64-bit integer to store the result.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_get_serial_number(uint64_t* serial_number);


/**
* @brief Calculates the CRC checksum.
* @param data Pointer to the data buffer.
* @param len Length of the data buffer.
*/
uint8_t scd41_crc_calculate(const uint8_t* data, size_t len);

/**
 * @brief Reads the latest measurement data (CO2, Temp, Humidity).
 * @param measurement A pointer to a struct to store the results.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_read_measurement(scd41_measurement_t* measurement);

/**
 * @brief Starts periodic measurements. The sensor will take a new reading 
 *        every 5 seconds.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_start_periodic_measurement(void);


/**
 * @brief Stops periodic measurements.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_stop_periodic_measurement(void);

/**
 * @brief Checks if a measurement is ready to be read.
 * @param is_data_ready Pointer to a boolean that will be set to true if data is ready.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_get_data_ready_status(bool* is_data_ready);

/**
 * @brief Triggers a single measurement and reads the result. 
 *        This is a blocking operation that takes ~5 seconds.
 * @param measurement A pointer to a struct to store the results.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_measure_single_shot(scd41_measurement_t* measurement);

/**
 * @brief Set the sensor altitude to compensate for atmospheric pressure.    
 * @param altitude_m Altitude in meters (m) above sea level.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_set_sensor_altitude(uint16_t altitude_m);

/**
 * @brief Gets the currentlly configured sensor altitude.
 * @param altitude_m Pointer to store the altitude.
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_get_sensor_altitude(uint16_t* altitude_m);

/**
 * @brief Sets the sensor ambient pressure.
 * @param pressure_pa The ambient pressure in Pascal (Pa).
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_set_ambient_pressure(uint32_t pressure_pa);

/**
 * @brief Gets the sensor ambient pressure.
 * @param pressure_pa Pointer to pressure in Pascal (Pa).
 * @return SCD41_OK on success, non-zero on failure.
 */
int8_t scd41_get_ambient_pressure(uint32_t* pressure_pa);

#endif // SCD41_H
