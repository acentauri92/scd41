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

    mock().expectOneCall("delay_ms").withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    // ACT
    int8_t result = scd41_get_serial_number(&actual_serial);

    // ASSERT
    LONGS_EQUAL(SCD41_OK, result);
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
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_bad_crc_response, sizeof(fake_bad_crc_response))
          .andReturnValue(0);

    // ACT
    int8_t result = scd41_get_serial_number(&actual_serial);

    CHECK_TRUE(result != SCD41_OK);
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
          .withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    // ACT
    int8_t result = scd41_read_measurement(&actual_measurement);

    // ASSERT
    // This test will fail because our dummy function returns -1.
    LONGS_EQUAL(SCD41_OK, result);
    LONGS_EQUAL(500, actual_measurement.co2_ppm);
    DOUBLES_EQUAL(25.0, actual_measurement.temperature_c, 0.1);
    DOUBLES_EQUAL(37.0, actual_measurement.humidity_rh, 0.1);
}


TEST (Scd41Driver_TestGroup, Scd41StartPeriodicMeasurement_Success) {
    uint8_t expected_cmd[] = {0x21, 0xB1};

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_START_PERIODIC_MEAS_DELAY_MS);

    int8_t result = scd41_start_periodic_measurement();

    LONGS_EQUAL(SCD41_OK, result);
}

TEST (Scd41Driver_TestGroup, Scd41StopPeriodicMeasurement_Success) {
    uint8_t expected_cmd[] = {0x3F, 0x86};

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_STOP_PERIODIC_MEAS_DELAY_MS);

    int8_t result = scd41_stop_periodic_measurement();

    LONGS_EQUAL(0, result);
}

TEST (Scd41Driver_TestGroup, GetDataReadyStatus_Failure) {
    uint8_t expected_cmd[] = {0xE4, 0xB8};
    uint8_t fake_sensor_response[] = {0x80, 0x00, 0xA2};
    bool is_data_ready;

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 3)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    int8_t result = scd41_get_data_ready_status(&is_data_ready);

    LONGS_EQUAL(SCD41_OK, result);
    CHECK_FALSE(is_data_ready);
}

TEST (Scd41Driver_TestGroup, GetDataReadyStatus_Success) {
    uint8_t expected_cmd[] = {0xE4, 0xB8};
    // Change LSB to 0xFF and update CRC to mimic data ready
    uint8_t fake_sensor_response[] = {0x80, 0xFF, 0x0E};
    bool is_data_ready;

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_GET_DATA_READY_STATUS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 3)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    int8_t result = scd41_get_data_ready_status(&is_data_ready);

    LONGS_EQUAL(SCD41_OK, result);
    CHECK_TRUE(is_data_ready);
}

TEST (Scd41Driver_TestGroup, GetSingleshotMeasurement_success) {
    uint8_t single_shot_cmd[] = {0x21, 0x9D};
    uint8_t wakeup_command[] = {0x36, 0xF6};
    uint8_t read_meas_cmd[] = {0xEC, 0x05};
    uint8_t fake_sensor_response[] = {0x01, 0xF4, 0x33, 0x66, 0x67, 0xA2, 0x5E, 0xB9, 0x3C};
    scd41_measurement_t actual_measurement = {0, 0.0f, 0.0f};
    
    // Write data
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", single_shot_cmd, sizeof(single_shot_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_SINGLE_SHOT_MEAS_DELAY_MS);

    // Wakeup sensor
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", wakeup_command, sizeof(wakeup_command))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_WAKEUP_DELAY_MS);

    // Read data
    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", read_meas_cmd, sizeof(read_meas_cmd))
          .withParameter("len", 2)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

    mock().expectOneCall("i2c_read")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withParameter("len", 9)
          .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
          .andReturnValue(0);

    int8_t result = scd41_measure_single_shot(&actual_measurement);

    LONGS_EQUAL(SCD41_OK, result);
    LONGS_EQUAL(500, actual_measurement.co2_ppm);
    DOUBLES_EQUAL(25.0, actual_measurement.temperature_c, 0.1);
    DOUBLES_EQUAL(37.0, actual_measurement.humidity_rh, 0.1);
}


TEST(Scd41Driver_TestGroup, SetSensorAltitude_Success) {
    // Write command and the altitude. Sample values from datasheet
    // section 3.7.3
    uint8_t set_altitude_command[] = {0x24, 0x27, 0x07, 0x9E, 0x09};
    uint16_t altitude_to_set = 1950;

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", set_altitude_command, sizeof(set_altitude_command))
          .withParameter("len", 5)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_SET_SENSOR_ALTITUDE_DELAY_MS);

    int8_t result = scd41_set_sensor_altitude(altitude_to_set);
    LONGS_EQUAL(SCD41_OK, result);

}

TEST(Scd41Driver_TestGroup, GetSensorAltitude_Success) {

    uint16_t actual_altitude = 0;
    uint16_t expected_altitude = 1100;
    uint8_t expected_cmd[] = {0x23, 0x22};

//     Sample values from datasheet section 3.7.4
    uint8_t fake_sensor_response[] = {0x04, 0x4C, 0x42};

      mock().expectOneCall("i2c_write")
            .withParameter("addr", SCD41_I2C_ADDR)
            .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
            .withParameter("len", 2)
            .andReturnValue(0);

      mock().expectOneCall("delay_ms")
            .withParameter("ms", SCD41_READ_MEAS_DELAY_MS);

      mock().expectOneCall("i2c_read")
            .withParameter("addr", SCD41_I2C_ADDR)
            .withParameter("len", 3)
            .withOutputParameterReturning("data", fake_sensor_response, 
                                          sizeof(fake_sensor_response))
            .andReturnValue(0);

      int8_t result = scd41_get_sensor_altitude(&actual_altitude);
      LONGS_EQUAL(SCD41_OK, result);
      LONGS_EQUAL(expected_altitude, actual_altitude);

}

TEST(Scd41Driver_TestGroup, SetSensorAmbientPressure_Success) {
    // Write command and the pressure. Sample values from datasheet
    // section 3.7.5 
    uint8_t set_pressure_command[] = {0xE0, 0x00, 0x03, 0xDB, 0x42};
    uint32_t pressure_to_set = 98700;

    mock().expectOneCall("i2c_write")
          .withParameter("addr", SCD41_I2C_ADDR)
          .withMemoryBufferParameter("data", set_pressure_command, sizeof(set_pressure_command))
          .withParameter("len", 5)
          .andReturnValue(0);

    mock().expectOneCall("delay_ms")
          .withParameter("ms", SCD41_SET_AMBIENT_PRESSURE_DELAY_MS);

    int8_t result = scd41_set_ambient_pressure(pressure_to_set);
    LONGS_EQUAL(SCD41_OK, result);

}

TEST(Scd41Driver_TestGroup, GetSensorAmbientPressure_Success) {
      // Write command and the pressure. Sample values from datasheet
      // section 3.7.5 
      uint32_t actual_pressure_pa = 0;
      uint32_t expected_pressure_pa = 98700;

      uint8_t expected_cmd[] = {0xE0, 0x00};
      // The sensor returns the pressure in mbar (Pa / 100), so 1013 (0x03F5).
      uint8_t fake_sensor_response[] = {0x03, 0xDB, 0x42};

      mock().expectOneCall("i2c_write")
            .withParameter("addr", SCD41_I2C_ADDR)
            .withMemoryBufferParameter("data", expected_cmd, sizeof(expected_cmd))
            .withParameter("len", 2)
            .andReturnValue(0);

      mock().expectOneCall("delay_ms").withParameter("ms", SCD41_GET_AMBIENT_PRESSURE_DELAY_MS);

      mock().expectOneCall("i2c_read")
            .withParameter("addr", SCD41_I2C_ADDR)
            .withParameter("len", 3)
            .withOutputParameterReturning("data", fake_sensor_response, sizeof(fake_sensor_response))
            .andReturnValue(0);

      int8_t result = scd41_get_ambient_pressure(&actual_pressure_pa);

      LONGS_EQUAL(SCD41_OK, result);
      UNSIGNED_LONGS_EQUAL(expected_pressure_pa, actual_pressure_pa);

}

int main(int ac, char** av)
{
    return CommandLineTestRunner::RunAllTests(ac, av);
}
