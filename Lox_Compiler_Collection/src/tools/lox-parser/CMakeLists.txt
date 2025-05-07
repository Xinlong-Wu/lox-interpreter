add_executable(lox-parser
    lox-parser.cpp
)

target_link_libraries(lox-parser
#    LoxVM
#    LoxCompiler
    LoxParser
    LoxToken
    LoxScanner
    LoxDebug
#    LoxUtils
)
