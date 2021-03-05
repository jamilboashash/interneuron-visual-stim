cmake_minimum_required(VERSION 3.17)
project(led_protocol C)

set(CMAKE_C_STANDARD 99)

include_directories(/usr/lib/avr/include/)

add_executable(led_protocol main.c)