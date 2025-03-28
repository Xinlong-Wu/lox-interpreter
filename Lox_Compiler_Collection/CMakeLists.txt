cmake_minimum_required(VERSION 3.10)
project(LCC C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(USE_INTERPRETER "Use interpreter" ON)
if(USE_INTERPRETER)
    message(STATUS "Using interpreter")
    set(LOX_TARGET "lox-interpreter")
else()
    message(STATUS "Using clox")
    set(LOX_TARGET "clox")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests    # tests 源码路径
    ${CMAKE_BINARY_DIR}/tests               # 构建目录下的 tests 路径
)

add_subdirectory(src)
