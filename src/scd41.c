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
 * @retun SCD41_OK on success, non-zero on CRC failure.
 */
static int8_t _scd41_read_word_with_crc(const uint8_t* buffer, uint16_t* word) {
    if(scd41_crc_calculate(buffer, 2) != buffer[2]) {
        return SCD41_ERR_CRC;
    }
    // Combine MSB and LSB
    *word = ((uint16_t)buffer[0] << 8) | buffer[1];
    return SCD41_OK;
}

/**
 * @brief Internal helper to send a command with no subsequent read.
 * @param command The 16-bit command to send.
 * @param delay_after_ms Milliseconds to wait after sending the command.
 * @retun SCD41_OK on success, non-zero on failure.
 */
static int8_t _scd41_send_command(uint16_t command, uint32_t delay_after_ms) {
    uint8_t cmd_buffer[2];
    scd41_fill_command_buffer(command, cmd_buffer);

    if (i2c_write(SCD41_I2C_ADDR, cmd_buffer, 2) != 0)
        return SCD41_ERR_I2C_WRITE;

    if (delay_after_ms > 0)
        delay_ms(delay_after_ms);

    return SCD41_OK;
}

/**
 * @brief Helper function to send command and read back a 16 bit word with CRC.
 * @param command The 16 bit command to send.
 * @param word Pointer to buffer to store the result.
 * @param delay_ms Time to wait after sending the command.
 * @retun SCD41_OK on succcess, non-zero on failure.
 */

 int8_t _scd41_read_u16_with_crc(uint16_t command, uint16_t* word, uint32_t delay_ms) {
    uint8_t read_buffer[3];
    int8_t result;

    result = _scd41_send_command(command, delay_ms);
    if(result != SCD41_OK)
        return result;

    if (i2c_read(SCD41_I2C_ADDR, read_buffer, 3) != 0)
        return SCD41_ERR_I2C_READ;

    return _scd41_read_word_with_crc(read_buffer, word);
 }
 
/**
 * @brief Internal helper to send a command and read a 9-byte response.
 * @param command The 16-bit command to send.
 * @param read_buffer A pointer to a 9-byte buffer to store the response.
 * @retun SCD41_OK on success, non-zero on failure.
 */
static int8_t _scd41_send_command_and_read(uint16_t command, uint8_t* read_buffer) {
    uint8_t cmd_buffer[2];

    // Prepare and send the command to the sensor
    scd41_fill_command_buffer(command, cmd_buffer);
    if (i2c_write(SCD41_I2C_ADDR, cmd_buffer, 2) != 0) {
        return SCD41_ERR_I2C_WRITE; 
    }

    // Sensor needs ~1ms to respond with data
    delay_ms(SCD41_READ_MEAS_DELAY_MS);

    // Read the 9-byte response from the sensor
    if (i2c_read(SCD41_I2C_ADDR, read_buffer, 9) != 0) {
        return SCD41_ERR_I2C_READ;
    }
   
}


int8_t scd41_reinit(void) {
    return _scd41_send_command(SCD41_CMD_REINIT, SCD41_REINIT_DELAY_MS);

}

int8_t scd41_wakeup(void) {
    return _scd41_send_command(SCD41_CMD_WAKEUP, SCD41_WAKEUP_DELAY_MS);
}

int8_t scd41_get_serial_number(uint64_t* serial_number) {

    uint8_t read_buffer[9];
    uint16_t word1, word2, word3;
    int8_t result = SCD41_OK;

    result = _scd41_send_command_and_read(SCD41_CMD_GET_SERIAL_NUMBER, read_buffer);
    if(result != SCD41_OK)
        return result;

    // Validate each word
    result = _scd41_read_word_with_crc(&read_buffer[0], &word1) != 0 ||
                _scd41_read_word_with_crc(&read_buffer[3], &word2) != 0 ||
                _scd41_read_word_with_crc(&read_buffer[6], &word3) != 0;

    if(result != SCD41_OK)
        return result;

    // Combine the three 16-bit words into a single 48-bit serial number.
    *serial_number = ((uint64_t)word1 << 32) | ((uint64_t)word2 << 16) | 
                        (uint64_t)word3;

    return result;
}

int8_t scd41_read_measurement(scd41_measurement_t* measurement) {

    uint8_t read_buffer[9];
    uint16_t co2_raw, temp_raw, rh_raw;
    int8_t result = SCD41_OK;

    result = _scd41_send_command_and_read(SCD41_CMD_READ_MEAS, read_buffer) != 0;
    if(result != SCD41_OK)
        return result;
    
    // Validate each word
    result = _scd41_read_word_with_crc(&read_buffer[0], &co2_raw) != 0 ||
                _scd41_read_word_with_crc(&read_buffer[3], &temp_raw) != 0 ||
                _scd41_read_word_with_crc(&read_buffer[6], &rh_raw) != 0;

    if(result != SCD41_OK)
        return result; 

    // Formulas are from datasheet Section 3.6.2
    measurement->co2_ppm = co2_raw;
    measurement->temperature_c = -45.0f + ( (175.0f * (float)temp_raw) / 65535.0f );
    measurement->humidity_rh = 100.0f * (float)rh_raw / 65535.0f;

    return result;
}

int8_t scd41_start_periodic_measurement(void) {
    
    return _scd41_send_command(SCD41_CMD_START_PERIODIC_MEAS, 
                                SCD41_START_PERIODIC_MEAS_DELAY_MS);
}

int8_t scd41_stop_periodic_measurement(void){

   return _scd41_send_command(SCD41_CMD_STOP_PERIODIC_MEAS, 
                                SCD41_STOP_PERIODIC_MEAS_DELAY_MS);
}

int8_t scd41_get_data_ready_status(bool* is_data_ready){

    uint16_t status_word;

    int8_t result = _scd41_read_u16_with_crc(SCD41_CMD_GET_DATA_READY_STATUS, 
                                            &status_word,
                                            SCD41_GET_DATA_READY_STATUS_DELAY_MS);

    if (result != SCD41_OK)
        return result;

    if(status_word & 0x07FF)
        *is_data_ready = true;
    else
        *is_data_ready = false; 

    return result;
}

int8_t scd41_measure_single_shot(scd41_measurement_t* measurement) {

    int8_t result = _scd41_send_command(SCD41_CMD_SINGLE_SHOT_MEAS, 
                                        SCD41_SINGLE_SHOT_MEAS_DELAY_MS);
    if (result != SCD41_OK)
        return result;

    // The sensor may have gone back to sleep. Wake it up before reading.
    if (scd41_wakeup() != SCD41_OK)
        return SCD41_ERR_WAKEUP;

    return scd41_read_measurement(measurement);
}

int8_t scd41_set_sensor_altitude(uint16_t altitude_m) {
    uint8_t packet[5];

    scd41_fill_command_buffer(SCD41_CMD_SET_SENSOR_ALTITUDE, &packet[0]);

    // Fill the 2-byte altitude argument
    packet[2] = (uint8_t)(altitude_m >> 8);
    packet[3] = (uint8_t)(altitude_m & 0xFF);

    packet[4] = scd41_crc_calculate(&packet[2], 2);

    if (i2c_write(SCD41_I2C_ADDR, packet, 5) != 0)
        return SCD41_ERR_I2C_WRITE;

    delay_ms(SCD41_SET_SENSOR_ALTITUDE_DELAY_MS);

    return SCD41_OK;
}

int8_t scd41_get_sensor_altitude(uint16_t* altitude_m) {

   return _scd41_read_u16_with_crc(SCD41_CMD_GET_SENSOR_ALTITUDE, altitude_m,
                                    SCD41_GET_SENSOR_ALTITUDE_DELAY_MS);

};

