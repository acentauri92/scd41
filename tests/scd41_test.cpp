#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
    #include "scd41.h"
    #include "i2c_hal.h"
}

// --- Mock Implementations ---
int8_t i2c_write(uint8_t addr, const uint8_t* data, size_t len) {
    return mock().actualCall("i2c_write")
                 .withParameter("addr", addr)
                 .withMemoryBufferParameter("data", data, len)
                 .withParameter("len", len)
                 .returnIntValue();
}

int8_t i2c_read(uint8_t addr, uint8_t* data, size_t len) {
    return mock().actualCall("i2c_read")
                 .withParameter("addr", addr)
                 .withOutputParameter("data", data)
                 .withParameter("len", len)
                 .returnIntValue();
}

void delay_ms(uint32_t ms) {
    mock().actualCall("delay_ms").withParameter("ms", ms);
}


// --- Test Group ---
TEST_GROUP(Scd41Driver_TestGroup)
{
    void teardown()
    {
        // This checks that all expected mock calls were made and cleans up.
        mock().checkExpectations();
        mock().clear();
    }
};

// --- Test Cases ---
TEST(Scd41Driver_TestGroup, FillCommandBuffer_CorrectlyFormatsBytes)
{
    uint16_t command_to_send = 0x202f;
    uint8_t expected_output_buffer[2] = {0x20, 0x2f};
    uint8_t actual_output_buffer[2];
    scd41_fill_command_buffer(command_to_send, actual_output_buffer);
    MEMCMP_EQUAL(expected_output_buffer, actual_output_buffer, 2);
}

TEST(Scd41Driver_TestGroup, GetSerialNumber_Success)
{
    // ARRANGE
    uint64_t actual_serial = 0;
    uint64_t expected_serial = 0x66BFEF073BF2;
    uint8_t expected_cmd[] = {0x36, 0x82};
    uint8_t fake_sensor_response[] = {0x66, 0xBF, 0x1F, 0xEF, 0x07, 0xF9, 0x3B, 0xF2, 0xC9};

    // SET EXPECTATIONS
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms").withParameter("ms", 1);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    // ACT
    int8_t result = scd41_get_serial_number(&actual_serial);

    // ASSERT
    LONGS_EQUAL(0, result);
    UNSIGNED_LONGLONGS_EQUAL(expected_serial, actual_serial);
}

TEST(Scd41Driver_TestGroup, CrcCalculationIsCorrect) {
    uint8_t data[] = {0xBE, 0xEF};
    uint8_t expected_crc = 0x92;

    uint8_t actual_crc = scd41_crc_calculate(data, 2);

    LONGS_EQUAL(expected_crc, actual_crc);
}

TEST(Scd41Driver_TestGroup, GetSerialNumber_FailsOnBadCrc)
{
    // ARRANGE
    uint64_t actual_serial = 0;
    uint8_t expected_cmd[] = {0x36, 0x82};
    
    // This response has an incorrect CRC for the second word (should be 0xF9)
    uint8_t fake_bad_crc_response[] = {0x66, 0xBF, 0x1F, 0xEF, 0x07, 0xFF, 0x3B, 0xF2, 0xC9};

    // SET EXPECTATIONS (the driver should still read the data)
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2).andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", 1);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_bad_crc_response, sizeof(fake_bad_crc_response)).andReturnValue(0);

    // ACT
    int8_t result = scd41_get_serial_number(&actual_serial);

    CHECK_TRUE(result != 0);
}

TEST(Scd41Driver_TestGroup, ReadMeasurement_Success)
{
    // ARRANGE: Define fake raw data and the expected final values.
    // Sample raw values are from the example in the datasheet. Section 3.6.2
    // CO2=500, Temp=25.0C, RH=37.0%
    uint8_t fake_sensor_response[] = {0x01, 0xF4, 0x33, 0x66, 0x67, 0xA2, 0x5E, 0xB9, 0x3C};
    uint8_t expected_cmd[] = {0xEC, 0x05};
    scd41_measurement_t actual_measurement = {0, 0.0f, 0.0f};

    // SET EXPECTATIONS
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", 1);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    // ACT
    int8_t result = scd41_read_measurement(&actual_measurement);

    // ASSERT
    // This test will fail because our dummy function returns -1.
    LONGS_EQUAL(0, result);
    LONGS_EQUAL(500, actual_measurement.co2_ppm);
    DOUBLES_EQUAL(25.0, actual_measurement.temperature_c, 0.1);
    DOUBLES_EQUAL(37.0, actual_measurement.humidity_rh, 0.1);
}

int main(int ac, char** av)
{
    return CommandLineTestRunner::RunAllTests(ac, av);
}
