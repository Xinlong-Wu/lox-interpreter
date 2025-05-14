#include "compiler/Parser.h"

#include<iostream>

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

static int runFile(const char *path)
{
    char *source = readFile(path);
    lox::Parser parser = lox::Parser(source);

    parser.advance();

    while (parser.hasNext())
    {
        std::unique_ptr<lox::StmtBase> stmt = parser.parseDeclaration();
        if (stmt != nullptr){
            stmt->dump();
        }
    }

    free(source);

    if (parser.hasError())
    {
        return 65;
    }
    return 0;
}

static void repl()
{
    char line[1024];
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
        if (stmt != nullptr){
            stmt->dump();
        }
        std::cout << std::endl;
    }
}

int main(int argc, char const *argv[])
{
    if (argc == 1)
    {
        repl();
    }
    else if (argc == 2) {
        return runFile(argv[1]);
    }
    else {
        fprintf(stderr, "Usage: lox-parser [path]\n");
        exit(64);
    }
    return 0;
}

