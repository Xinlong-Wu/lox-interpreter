
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    Parser.cpp
    PrecedenceParser.cpp
    StatementBuilder.cpp
)

add_library(LoxToken STATIC
    Token.cpp
)

add_library(LoxScanner STATIC
    Scanner.cpp
)
