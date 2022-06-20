cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)


project(blink C CXX ASM)

set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

pico_sdk_init()

add_executable(main
	main.c
	)

target_link_libraries(main pico_stdlib hardware_gpio hardware_flash hardware_irq hardware_sync hardware_i2c hardware_rtc hardware_adc)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

pico_add_extra_outputs(main)
