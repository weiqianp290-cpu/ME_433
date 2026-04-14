cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hw4c C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hw4c
    hw4c.c
    ssd1306.c
)

target_include_directories(hw4c PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(hw4c
    pico_stdlib
    hardware_i2c
)

pico_enable_stdio_usb(hw4c 1)
pico_enable_stdio_uart(hw4c 0)

pico_add_extra_outputs(hw4c)