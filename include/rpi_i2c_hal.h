#ifndef RPI_I2C_HAL_H
#define RPI_I2C_HAL_H

/**
 * @brief Initializes the I2C bus on the Raspberry Pi.
 * @return 0 on success, non-zero on failure.
 */
int8_t rpi_i2c_hal_init(void);

/**
 * @brief Closes the I2C bus file descriptor.
 */
void rpi_i2c_hal_close(void);

#endif // RPI_I2C_HAL_H
