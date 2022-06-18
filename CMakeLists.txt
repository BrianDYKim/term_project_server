cmake_minimum_required(VERSION 3.15)
project(term_project_server C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS -pthread)

add_executable(term_project_server main.c client_info.c client_info.h)