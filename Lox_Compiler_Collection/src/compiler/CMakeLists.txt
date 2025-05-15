
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    # AST
    AST/ASTVisitor.cpp

    # SemanticAnalyzer
    SemanticAnalyzer/SemanticAnalyzer.cpp

    Parser/Parser.cpp
    Parser/PrecedenceParser.cpp
    Parser/StatementBuilder.cpp
)

# add_library(LoxIR STATIC
#     IR/IRBuilder.cpp
# )

add_library(LoxToken STATIC
    Scanner/Token.cpp
)

add_library(LoxScanner STATIC
    Scanner/Scanner.cpp
)
