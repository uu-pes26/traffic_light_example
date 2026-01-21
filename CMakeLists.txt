cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2)
include(pico_sdk_import.cmake)
# Bellow makes CMake output compile commands that are used by IDEs, e.g., Visual Studio Code
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(traffic_light_simple)
project(traffic_light_queue)
project(traffic_light_table_driven)

pico_sdk_init()

add_executable(traffic_light_simple src/main_simple.c)
pico_add_extra_outputs(traffic_light_simple)
target_link_libraries(traffic_light_simple pico_stdlib)

add_executable(traffic_light_queue src/main_queue.c)
pico_add_extra_outputs(traffic_light_queue)
target_link_libraries(traffic_light_queue pico_stdlib)

add_executable(traffic_light_table_driven src/main_table_driven.c)
pico_add_extra_outputs(traffic_light_table_driven)
target_link_libraries(traffic_light_table_driven pico_stdlib)
