
add_library(LoxCompiler STATIC
    Compiler.cpp
)

add_library(LoxParser STATIC
    # ErrorReporter
    ErrorReporter.cpp

    # AST
    AST/ASTVisitor.cpp

    # SemanticAnalyzer
    SemanticAnalyzer/SemanticAnalyzer.cpp
    SemanticAnalyzer/SymbolTable.cpp

    Parser/Parser.cpp
    Parser/PrecedenceParser.cpp
    Parser/StatementBuilder.cpp
)

# add_library(LoxIR STATIC
#     IR/IRBuilder.cpp
# )

add_library(LoxScanner STATIC
    Scanner/Scanner.cpp
)
