cmake_minimum_required(VERSION 3.14)

project(SCD41_App LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

# --- Add CppUTest Submodule ---
# This builds the CppUTest library from our submodule.
add_subdirectory(lib/cpputest)

# --- Project Source Files ---
# Create a library for our SCD41 driver logic.
add_library(scd41_driver src/scd41.c)
target_include_directories(scd41_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# --- Test Executable ---
# Create an executable for our tests.
add_executable(run_tests tests/scd41_test.cpp)

# Link our test executable against our driver and the CppUTest library.
target_link_libraries(run_tests PRIVATE scd41_driver CppUTest CppUTestExt)

# --- CTest Integration ---
# Enable and add the test so we can run it with 'make test'.
enable_testing()
add_test(NAME scd41_unit_tests COMMAND run_tests)
