cmake_minimum_required(VERSION 3.22)
project(mag-usb C)

set(CMAKE_C_STANDARD 11)

add_executable(mag-usb src/main.c
        src/cmdmgr.c
        src/magdata.c
        src/i2c.c
        src/i2c-pololu.c
        src/config.h
        src/config.c
        src/sensor_tests.c
        src/sensor_tests.h)

# Unit tests for i2c-pololu
add_executable(i2c-pololu-tests
        tests/test_i2c_pololu.c
        src/i2c-pololu.c)

target_include_directories(i2c-pololu-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
