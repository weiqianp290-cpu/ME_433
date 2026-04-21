cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(hw6c C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hw6c
    hw6c.c
    usb_descriptors.c
)

target_include_directories(hw6c PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(hw6c
    pico_stdlib
    tinyusb_device
    tinyusb_board
    hardware_i2c
    hardware_gpio
    m
)

pico_enable_stdio_usb(hw6c 0)
pico_enable_stdio_uart(hw6c 0)

pico_add_extra_outputs(hw6c)