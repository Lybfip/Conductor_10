# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type" FORCE)

# Pico SDK の導入
include(pico_sdk_import.cmake)

project(midi_to_hid_ps C CXX ASM)

# Raspberry Pi Pico SDK の初期化
pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/extern/pico-pio-usb ${CMAKE_CURRENT_BINARY_DIR}/pico_pio_usb)

# Add executable. Default name is the project name, version 0.1

add_executable(midi_to_hid_ps
    src/main.cpp
    src/hid_keyboard.cpp
    src/keymap.cpp
    src/usb_host.cpp
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

set_target_properties(midi_to_hid_ps PROPERTIES OUTPUT_NAME conductor_10)

pico_set_program_name(midi_to_hid_ps "midi_to_hid_ps")
pico_set_program_version(midi_to_hid_ps "0.1")

# Generate PIO header
pico_generate_pio_header(midi_to_hid_ps ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Modify the below lines to enable/disable output over UART/USB
# UART is used for debug logs because the Pico USB port is reserved for HID device output.
pico_enable_stdio_usb(midi_to_hid_ps 0)
pico_enable_stdio_uart(midi_to_hid_ps 1)



# Add the standard library to the build
target_link_libraries(midi_to_hid_ps
    pico_stdlib
    pico_pio_usb)

# Add the standard include files to the build
target_include_directories(midi_to_hid_ps PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(midi_to_hid_ps 
        tinyusb_host
    tinyusb_device
        hardware_pio
        hardware_timer
        )

target_compile_definitions(midi_to_hid_ps PRIVATE
    PIO_USB_USE_TINYUSB
    PICO_DEFAULT_PIO_USB_DP_PIN=0
    PICO_DEFAULT_UART=1
    PICO_DEFAULT_UART_TX_PIN=8
    PICO_DEFAULT_UART_RX_PIN=9
    PICO_DEFAULT_PIO_USB_VBUSEN_PIN=22
    PICO_DEFAULT_PIO_USB_VBUSEN_STATE=1)

pico_add_extra_outputs(midi_to_hid_ps)

