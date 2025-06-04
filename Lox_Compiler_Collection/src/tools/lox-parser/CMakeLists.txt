add_executable(lox-parser
    lox-parser.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(lox-parser PRIVATE -g -O0 -fstandalone-debug)
endif()

target_link_libraries(lox-parser
#    LoxVM
#    LoxCompiler
    LoxParser
    LoxScanner
    LoxDebug
#    LoxUtils
)
