#include "compiler/Parser.h"
#include "compiler/Token.h"
#include <sstream>
#include <unordered_map>

namespace lox
{
    void Parser::advance()
    {
        Token nextToken = scanner.next();
        if (nextToken == TokenType::TOKEN_ERROR)
        {
            TokenError errorToken = TokenError(nextToken);
            if (!errorToken.hasErrorMsg())
            {
                errorToken.setErrorMsg("Unexpected character.");
            }
            this->parseError(errorToken);
        }
        previousToken = currentToken;
        currentToken = nextToken;
    }

    void Parser::synchronize()
    {
        if (!panicMode)
            return;

        panicMode = false;

        while (currentToken != TokenType::TOKEN_EOF)
        {
            if (previousToken == TokenType::TOKEN_SEMICOLON)
                return;

            switch (previousToken.getType())
            {
            case TokenType::TOKEN_CLASS:
            case TokenType::TOKEN_FUN:
            case TokenType::TOKEN_VAR:
            case TokenType::TOKEN_FOR:
            case TokenType::TOKEN_IF:
            case TokenType::TOKEN_WHILE:
            // case TokenType::TOKEN_PRINT:
            case TokenType::TOKEN_RETURN:
                return;

            default:;
            }

            advance();
        }
    }

    void Parser::parse(TokenType type, std::optional<std::string> errorMsg) {
        if (parseOptional(type)) {
            return;
        }
        if (!errorMsg.has_value()) {
            parseError(type);
            return;
        }
        parseError(TokenError(currentToken, *errorMsg));
    }

    bool Parser::parseOptional(TokenType type) {
        if (!match(type)) return false;
        advance();
        panicMode = false;
        return true;
    }

    void Parser::parseError(TokenType type, bool shouldPanic) {
        if (panicMode) return;
        if (shouldPanic) panicMode = true;
        std::ostringstream os;
        os << previousToken.getLoction()
            << " Expect token "
            << Token(type)
            << " after "
            << previousToken
            << ", but got: "
            << currentToken;
        error = true;
        std::cerr << os.str() << std::endl;
    }

    void Parser::parseError(const Token &token, bool shouldPanic) {
        if (token != TokenType::TOKEN_ERROR) {
            this->parseError(token.getType(), shouldPanic);
            return;
        }

        if (panicMode) return;
        if (shouldPanic) panicMode = true;
        std::ostringstream os;
        os << previousToken.getLoction()
            << " Error: "
            << TokenError(token).getErrorMsg()
            << " at "
            << token;
        if (token != currentToken) {
            os  << ", after"
                << currentToken;
        }
        error = true;
        std::cerr << os.str() << std::endl;
    }

    void Parser::parseError(std::string_view message, bool shouldPanic) {
        if (panicMode) return;
        if (shouldPanic) panicMode = true;
        std::ostringstream os;
        os << previousToken.getLoction()
            << " Error: "
            << message
            << " at "
            << previousToken;
        if (currentToken != TokenType::TOKEN_ERROR) {
            os  << ", before: "
            << currentToken;
        }
        error = true;
        std::cerr << os.str() << std::endl;
    }

    void Parser::parseError(const std::unique_ptr<ExprBase> &expr, std::string_view message, bool shouldPanic) {
        if (panicMode) return;
        if (shouldPanic) panicMode = true;
        std::ostringstream os;
        os << expr->getLoc()
            << " Error: "
            << message
            << " at ";
        expr->print(os);
        os  << ", before: "
        << previousToken;
        error = true;
        std::cerr << os.str() << std::endl;
    }
} // namespace lox
