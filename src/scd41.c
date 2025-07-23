#include "scd41.h"
#include "i2c_hal.h" 

void scd41_fill_command_buffer(uint16_t command, uint8_t* buffer) {
    // Extract the MSB
    buffer[0] = (uint8_t)(command >> 8);

    // Extract the LSB
    buffer[1] = (uint8_t)(command & 0xFF);
}

uint8_t scd41_crc_calculate(const uint8_t* data, size_t len) {
    // CRC-8 parameters and algorithm from Sensirion datasheet (Section 3.12)
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

/**
 * @brief Internal helper to read a 16-bit word and verify its CRC.
 * @param buffer Pointer to the 3-byte data packet (MSB, LSB, CRC).
 * @param word Pointer to a uint16_t to store the result.
 * @return 0 on success, -1 on CRC failure.
 */
static int8_t _scd41_read_word_with_crc(const uint8_t* buffer, uint16_t* word) {
    if(scd41_crc_calculate(buffer, 2) != buffer[2]) {
        return -1;
    }
    // Combine MSB and LSB
    *word = ((uint16_t)buffer[0] << 8) | buffer[1];
    return 0;
}

/**
 * @brief Internal helper to send a command and read a 9-byte response.
 * @param command The 16-bit command to send.
 * @param read_buffer A pointer to a 9-byte buffer to store the response.
 * @return 0 on success, non-zero on failure.
 */
static int8_t _scd41_send_command_and_read(uint16_t command, uint8_t* read_buffer) {
    uint8_t cmd_buffer[2];

    // Prepare and send the command to the sensor
    scd41_fill_command_buffer(command, cmd_buffer);
    if (i2c_write(SCD41_I2C_ADDR, cmd_buffer, 2) != 0) {
        return -1; 
    }

    // Sensor needs ~1ms to respond with data
    delay_ms(1);

    // Read the 9-byte response from the sensor
    if (i2c_read(SCD41_I2C_ADDR, read_buffer, 9) != 0) {
        return -1; // Read error
    }
   
}

int8_t scd41_get_serial_number(uint64_t* serial_number) {

    uint8_t read_buffer[9];
    uint16_t word1, word2, word3;

    if (_scd41_send_command_and_read(SCD41_CMD_GET_SERIAL_NUMBER, read_buffer) != 0)
        return -1;

    // Validate each word
    if (_scd41_read_word_with_crc(&read_buffer[0], &word1) != 0 ||
        _scd41_read_word_with_crc(&read_buffer[3], &word2) != 0 ||
        _scd41_read_word_with_crc(&read_buffer[6], &word3) != 0)
            return -3;

    // Combine the three 16-bit words into a single 48-bit serial number.
    *serial_number = ((uint64_t)word1 << 32) | ((uint64_t)word2 << 16) | 
                        (uint64_t)word3;

    return 0;
}

int8_t scd41_read_measurement(scd41_measurement_t* measurement) {

    uint8_t read_buffer[9];
    uint16_t co2_raw, temp_raw, rh_raw;

    if (_scd41_send_command_and_read(SCD_41_CMD_READ_MEASUREMENT, read_buffer) != 0)
        return -1;
    
    // Validate each word
    if (_scd41_read_word_with_crc(&read_buffer[0], &co2_raw) != 0 ||
        _scd41_read_word_with_crc(&read_buffer[3], &temp_raw) != 0 ||
        _scd41_read_word_with_crc(&read_buffer[6], &rh_raw) != 0)
            return -3;
    
    // Formulas are from datasheet Section 3.6.2
    measurement->co2_ppm = co2_raw;
    measurement->temperature_c = -45.0f + ( (175.0f * (float)temp_raw) / 65535.0f );
    measurement->humidity_rh = 100.0f * (float)rh_raw / 65535.0f;

    return 0;
}

