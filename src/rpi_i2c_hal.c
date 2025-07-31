#include "i2c_hal.h"
#include "rpi_i2c_hal.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <stdio.h>

// A file descriptor for the I2C bus, kept private to this file.
static int i2c_file = -1;

int8_t rpi_i2c_hal_init(void) {
    const char *i2c_device = "/dev/i2c-1";
    if ((i2c_file = open(i2c_device, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return -1;
    }
    return 0;
}

void rpi_i2c_hal_close(void) {
    if (i2c_file != -1) {
        close(i2c_file);
    }
}

// The real hardware implementation of i2c_write
int8_t i2c_write(uint8_t addr, const uint8_t* data, size_t len) {
    if (ioctl(i2c_file, I2C_SLAVE, addr) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        return -1;
    }
    if (write(i2c_file, data, len) != len) {
        perror("Failed to write to the i2c bus");
        return -1;
    }
    return 0;
}

// The real hardware implementation of i2c_read
int8_t i2c_read(uint8_t addr, uint8_t* data, size_t len) {
    if (ioctl(i2c_file, I2C_SLAVE, addr) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        return -1;
    }
    if (read(i2c_file, data, len) != len) {
        perror("Failed to read from the i2c bus");
        return -1;
    }
    return 0;
}

// The real hardware implementation of delay_ms
void delay_ms(uint32_t ms) {
    usleep(ms * 1000);
}
