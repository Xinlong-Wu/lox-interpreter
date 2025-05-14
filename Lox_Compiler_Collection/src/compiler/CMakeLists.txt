
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    parser/Parser.cpp
    parser/PrecedenceParser.cpp
    parser/StatementBuilder.cpp
)

add_library(LoxToken STATIC
    scanner/Token.cpp
)

add_library(LoxScanner STATIC
    scanner/Scanner.cpp
)
