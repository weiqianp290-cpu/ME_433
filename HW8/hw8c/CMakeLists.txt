cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hw8c C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hw8c
    hw8c.c
)

target_link_libraries(hw8c
    pico_stdlib
    hardware_spi
    m
)

pico_enable_stdio_usb(hw8c 1)
pico_enable_stdio_uart(hw8c 0)

pico_add_extra_outputs(hw8c)