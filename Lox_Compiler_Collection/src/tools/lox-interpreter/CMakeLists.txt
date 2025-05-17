# CMakeLists.txt for the Lox interpreter tool
add_executable(lox-interpreter
    lox-interpreter.cpp
)

target_link_libraries(lox-interpreter
    LoxVM
    LoxCompiler
    LoxParser
    LoxScanner
    LoxDebug
    LoxUtils
)
