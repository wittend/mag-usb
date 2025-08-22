cmake_minimum_required(VERSION 4.0)
project(mag_usb C)

set(CMAKE_C_STANDARD 11)

add_executable(mag_usb src/main.c
        src/cmdmgr.c
        src/magdata.c
        src/i2c.c
        src/i2c_pololu.c)
