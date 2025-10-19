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
