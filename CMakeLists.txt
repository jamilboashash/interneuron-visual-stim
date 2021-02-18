cmake_minimum_required(VERSION 3.17)
project(pwm_avr C)

set(CMAKE_C_STANDARD 99)

include_directories(/usr/lib/avr/include/)

add_executable(pwm_avr main.c)