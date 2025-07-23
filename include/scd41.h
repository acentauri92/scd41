#ifndef SCD41_H
#define SCD41_H

#include <stdint.h>
#include <stddef.h>

#define SCD41_I2C_ADDR                  0x62
#define SCD41_CMD_GET_SERIAL_NUMBER     0x3682
#define SCD_41_CMD_READ_MEASUREMENT     0xEC05

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
 * @brief Reads the unique 48-bit serial number from the sensor.
 * @param serial_number A pointer to a 64-bit integer to store the result.
 * @return 0 on success, non-zero on failure.
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
 * @return 0 on success, non-zero on failure.
 */
int8_t scd41_read_measurement(scd41_measurement_t* measurement);

#endif // SCD41_H
