cmake_minimum_required(VERSION 3.22)
project(mag-usb C)

# Enforce the C standard explicitly
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Options for build hygiene
option(ENABLE_WERROR "Treat warnings as errors" OFF)
option(BUILD_TESTING "Enable building of tests" ON)

# Common warning flags (portable subset)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if (ENABLE_WERROR)
        add_compile_options(-Werror)
    endif ()
endif ()

add_executable(mag-usb
        src/main.c
        src/cmdmgr.c
        src/magdata.c
        src/i2c.c
        src/i2c-pololu.c
        src/config.c
        src/sensor_tests.c)

# Limit public include paths to src/
target_include_directories(mag-usb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Enable GNU/POSIX extensions needed for termios, clock_gettime, sigaction
target_compile_definitions(mag-usb PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)

# Unit tests for i2c-pololu
add_executable(i2c-pololu-tests
        tests/test_i2c_pololu.c
        src/i2c-pololu.c)

target_include_directories(i2c-pololu-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# Enable GNU/POSIX extensions for tests as well (sigaction, clock_gettime)
target_compile_definitions(i2c-pololu-tests PRIVATE _GNU_SOURCE _DEFAULT_SOURCE)

# CTest integration
if (BUILD_TESTING)
    include(CTest)
    add_test(NAME i2c-pololu-tests COMMAND i2c-pololu-tests)
endif ()
