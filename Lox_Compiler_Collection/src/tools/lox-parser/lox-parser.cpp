#include "Compiler/Parser/Parser.h"
#include "Compiler/SemanticAnalyzer/SymbolTable.h"
#include "Compiler/SemanticAnalyzer/SemanticAnalyzer.h"
#include "Compiler/ErrorReporter.h"

#include<iostream>
#include<cstring>

static lox::SymbolTable symbolTable;

static void initSymbolTable()
{
    symbolTable.enterScope();
    symbolTable.declare(std::make_shared<lox::FunctionSymbol>("print", std::vector<lox::Type>{lox::Type::TYPE_STRING}));
}

static char *readFile(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

static int runFile(const char *path, bool enableSemanticAnalyzer)
{
    char *source = readFile(path);
    lox::Parser parser = lox::Parser(source);
    initSymbolTable();
    lox::SemanticAnalyzer sa = lox::SemanticAnalyzer(symbolTable);

    parser.advance();

    while (parser.hasNext())
    {
        std::unique_ptr<lox::StmtBase> stmt = parser.parseDeclaration();
        if (stmt != nullptr){
            if (enableSemanticAnalyzer) {
                // Perform semantic analysis
                stmt->accept(sa);
            }
            stmt->dump();
        }
    }

    free(source);

    if (parser.hasError() || lox::ErrorReporter::hasError())
    {
        return 65;
    }
    return 0;
}

static void repl()
{
    char line[1024];
    initSymbolTable();
    lox::SemanticAnalyzer sa = lox::SemanticAnalyzer(symbolTable);
    for (;;)
    {
        printf("> ");

        if (!fgets(line, sizeof(line), stdin))
        {
            printf("\n");
            break;
        }

        lox::Parser parser = lox::Parser(line);

        parser.advance();

        std::unique_ptr<lox::StmtBase> stmt = parser.parseDeclaration();

        if (parser.hasError()) {
            continue;
        }

        if (stmt != nullptr){
            stmt->accept(sa);
            stmt->dump();
        }
        std::cout << std::endl;
    }
}

static void printUsage()
{
    fprintf(stderr, "Usage: lox-parser [path]\n");
    fprintf(stderr, "       lox-parser --semantic-analyzer [path]\n");
}

int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        repl();
    }
    else if (argc >= 2) {
        bool enableSemanticAnalyzer = false;
        // check flag --semantic-analyzer
        char const *filePath = argv[1];
        if (strcmp(argv[1], "--semantic-analyzer") == 0) {
            if (argc != 3) {
                printUsage();
                exit(64);
            }
            filePath = argv[2];
            enableSemanticAnalyzer = true;
        }
        return runFile(filePath, enableSemanticAnalyzer);
    }
    else {
        printUsage();
        exit(64);
    }
    return 0;
}

