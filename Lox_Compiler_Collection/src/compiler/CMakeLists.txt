
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    Parser.cpp
)

add_library(LoxToken STATIC
    Token.cpp
)

add_library(LoxScanner STATIC
    Scanner.cpp
)
