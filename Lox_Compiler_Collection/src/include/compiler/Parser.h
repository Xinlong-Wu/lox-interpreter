
#ifndef PARSER_H
#define PARSER_H

#include "compiler/Token.h"
#include "compiler/Scanner.h"

#include <optional>

namespace lox
{
    class Parser
    {
        private:
            Scanner scanner;
            Token currentToken;
            Token previousToken;
            bool error = false;
            bool panicMode = false;
        public:
            Parser(const char *source) : scanner(source) {};
            Parser(Scanner scanner) : scanner(scanner) {};
            ~Parser() {};

            Token &getCurrentToken() { return currentToken; };
            Token &getPreviousToken() { return previousToken; };

            void advance();
            bool match(TokenType type) { return currentToken == type; };
            bool hasNext() { return !match(lox::TokenType::TOKEN_EOF); };
            bool hasError() { return error; };
            bool isPanicMode() { return panicMode; };
            void synchronize();

            void parse(TokenType type, std::optional<std::string> errorMsg = std::nullopt);
            bool parseOptional(TokenType type);
            void parseError(TokenType tokenType, bool shouldPanic = true);
            void parseError(const Token &token, bool shouldPanic = true);
            void parseError(std::string_view message, bool shouldPanic = true);
    };
} // namespace lox

#endif // PARSER_H
