#ifndef I2C_HAL_H
#define I2C_HAL_H

#include <stdint.h>
#include <stddef.h> // for size_t

/**
 * @brief Writes data to the I2C bus.
 * @return 0 on success, non-zero on failure.
 */
int8_t i2c_write(uint8_t addr, const uint8_t* data, size_t len);

/**
 * @brief Reads data from the I2C bus.
 * @return 0 on success, non-zero on failure.
 */
int8_t i2c_read(uint8_t addr, uint8_t* data, size_t len);

/**
 * @brief Delays execution for a number of milliseconds.
 */
void delay_ms(uint32_t ms);


#endif // I2C_HAL_H