cmake_minimum_required(VERSION 4.0)
project(mag_usb C)

set(CMAKE_C_STANDARD 11)

add_executable(mag_usb main.c
        src/cmdmgr.c
        src/magdata.c
        src/i2c-pololu.c
        src/i2c-pololu.h
        src/i2c.c
        src/pololu_i2c.h
        src/pololu_i2c.c)
