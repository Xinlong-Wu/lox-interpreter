
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    Parser/Parser.cpp
    Parser/PrecedenceParser.cpp
    Parser/StatementBuilder.cpp
)

add_library(LoxToken STATIC
    Scanner/Token.cpp
)

add_library(LoxScanner STATIC
    Scanner/Scanner.cpp
)
