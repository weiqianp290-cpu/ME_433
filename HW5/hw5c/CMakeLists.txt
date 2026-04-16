cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hw5c C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hw5c
    hw5c.c
)

target_link_libraries(hw5c
    pico_stdlib
    hardware_i2c
)

pico_enable_stdio_usb(hw5c 1)
pico_enable_stdio_uart(hw5c 0)

pico_add_extra_outputs(hw5c)