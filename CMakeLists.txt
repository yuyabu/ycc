cmake_minimum_required(VERSION 3.14)
project(ycc)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(ycc
        CMakeLists.txt
        codegen.c
        container.c
        main.c
        parse.c
        ycc.h)