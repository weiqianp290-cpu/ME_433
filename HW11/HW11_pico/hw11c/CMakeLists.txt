cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico_hw11 C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico_hw11
    hw11c.c
)

target_link_libraries(pico_hw11
    pico_stdlib
    hardware_uart
)

pico_enable_stdio_usb(pico_hw11 1)
pico_enable_stdio_uart(pico_hw11 0)

pico_add_extra_outputs(pico_hw11)