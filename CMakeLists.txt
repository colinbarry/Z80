cmake_minimum_required(VERSION 3.14)
project(Z80)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()

add_library(z80 ./src/z80.c)
target_include_directories(z80 PUBLIC ./include)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
