cmake_minimum_required(VERSION 3.13)

# Import the Pico SDK
include(pico_sdk_import.cmake)

project(rp2040_usb C CXX ASM)
set(CMAKE_C_STANDARD 11)

# Initialize the SDK
pico_sdk_init()

add_executable(rp2040_usb
    main.c
    temp.c
    usb_descriptors.c
)

# Crucial: Tell the compiler to look in the current folder for tusb_config.h
target_include_directories(rp2040_usb PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Link necessary libraries
# tinyusb_device provides the USB stack
# tinyusb_board provides frequency/clock settings for the Pico hardware
target_link_libraries(rp2040_usb
    pico_stdlib
    tinyusb_device
    tinyusb_board
)

# Disable standard USB serial (CDC) to avoid conflicts with your Vendor class
pico_enable_stdio_usb(rp2040_usb 0)
pico_enable_stdio_uart(rp2040_usb 1)

# Create extra output formats (like .uf2 for easy dragging/dropping)
pico_add_extra_outputs(rp2040_usb)
