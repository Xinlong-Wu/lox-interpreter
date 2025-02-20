cmake_minimum_required(VERSION 3.10)
project(Clox C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

message(STATUS "CLOX SOURCES: ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(GLOB_RECURSE CLOX_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_executable(clox
    ${CLOX_SOURCES}
)