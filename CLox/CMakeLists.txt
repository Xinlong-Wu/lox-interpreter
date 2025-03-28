cmake_minimum_required(VERSION 3.10)
project(Clox C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(LOX_TARGET "clox")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

message(STATUS "CLOX SOURCES: ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(GLOB_RECURSE CLOX_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_executable(clox
    ${CLOX_SOURCES}
)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests    # tests 源码路径
    ${CMAKE_BINARY_DIR}/tests               # 构建目录下的 tests 路径
)

add_subdirectory(src/tools)
