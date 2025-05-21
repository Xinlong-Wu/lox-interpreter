#include "Compiler/Scanner/Scanner.h"

namespace lox
{
    Token Scanner::next()
    {
        if (!(inInterpolation && !isInterpolationStart))
            skipWhitespace();
        buffer = buffer.substr(current - buffer.begin());
        if (isAtEnd())
            return Token(TokenType::TOKEN_EOF, buffer.begin(), current, line, column);

        if (inInterpolation && !isInterpolationStart)
        {
            inInterpolation = false;
            return string();
        }

        char c = advance();
        if (isAlphaOrUnderScore(c))
            return identifier();
        if (isDigit(c))
            return number();

        switch (c)
        {
        case '(':
            return Token(TokenType::TOKEN_LEFT_PAREN, buffer.begin(), current, line, column);
        case ')':
            return Token(TokenType::TOKEN_RIGHT_PAREN, buffer.begin(), current, line, column);
        case '{':
            // return makeToken(TOKEN_LEFT_BRACE);
            return Token(TokenType::TOKEN_LEFT_BRACE, buffer.begin(), current, line, column);
        case '}':
            if (inInterpolation && isInterpolationStart)
            {
                isInterpolationStart = false;
                return Token(TokenType::TOKEN_INTERPOLATION_END, buffer.begin(), current, line, column);
            }
            return Token(TokenType::TOKEN_RIGHT_BRACE, buffer.begin(), current, line, column);
        case ';':
            return Token(TokenType::TOKEN_SEMICOLON, buffer.begin(), current, line, column);
        case ',':
            return Token(TokenType::TOKEN_COMMA, buffer.begin(), current, line, column);
        case '.':
            return Token(TokenType::TOKEN_DOT, buffer.begin(), current, line, column);
        case '-':
            return Token(TokenType::TOKEN_MINUS, buffer.begin(), current, line, column);
        case '+':
            return Token(TokenType::TOKEN_PLUS, buffer.begin(), current, line, column);
        case '/':
            return Token(TokenType::TOKEN_SLASH, buffer.begin(), current, line, column);
        case '*':
            return Token(TokenType::TOKEN_STAR, buffer.begin(), current, line, column);
        case ':':
            return Token(TokenType::TOKEN_COLON, buffer.begin(), current, line, column);
        case '!':
            return Token(match('=') ? TokenType::TOKEN_BANG_EQUAL : TokenType::TOKEN_BANG, buffer.begin(), current, line, column);
        case '=':
            return Token(match('=') ? TokenType::TOKEN_EQUAL_EQUAL : TokenType::TOKEN_EQUAL, buffer.begin(), current, line, column);
        case '<':
            return Token(match('=') ? TokenType::TOKEN_LESS_EQUAL : TokenType::TOKEN_LESS, buffer.begin(), current, line, column);
        case '>':
            return Token(match('=') ? TokenType::TOKEN_GREATER_EQUAL : TokenType::TOKEN_GREATER, buffer.begin(), current, line, column);
        case '"':
            return string();
        case '$':
            if (match('{') && inInterpolation)
            {
                return Token(TokenType::TOKEN_INTERPOLATION_START, buffer.begin(), current, line, column);
            }
            break;
        }
        return Token(TokenType::TOKEN_UNKNOWN, buffer.begin(), current, line, column);
    }

    bool Scanner::match(char expected)
    {
        if (isAtEnd())
            return false;
        if (*current != expected)
            return false;
        current++;
        return true;
    }

    // ====================== Private Methods ======================

    Token Scanner::string()
    {
        while (peek() != '"' && !isAtEnd())
        {
            if (peek() == '\n')
            {
                line++;
                column = 1;
            }

            if (peek() == '$' && peek(1) == '{')
            {
                if (inInterpolation)
                {
                    return TokenError("Nested interpolation is not allowed.", line, column);
                }
                inInterpolation = true;
                isInterpolationStart = true;
                break;
            }

            advance();
        }

        if (isAtEnd())
            return TokenError("Unterminated string.", line, column);

        // The closing quote.
        match('"');
        return Token(TokenType::TOKEN_STRING, buffer.begin(), current, line, column);
    }

    Token Scanner::identifier()
    {
        while (isAlphaOrUnderScore(peek()) || isDigit(peek()))
            advance();

        TokenType identifierType = TokenType::TOKEN_IDENTIFIER;
        std::string_view identifier = buffer.substr(0, current - buffer.begin());

        if (identifier == "and")
            identifierType = TokenType::TOKEN_AND;
        else if (identifier == "class")
            identifierType = TokenType::TOKEN_CLASS;
        else if (identifier == "else")
            identifierType = TokenType::TOKEN_ELSE;
        else if (identifier == "false")
            identifierType = TokenType::TOKEN_FALSE;
        else if (identifier == "for")
            identifierType = TokenType::TOKEN_FOR;
        else if (identifier == "fun")
            identifierType = TokenType::TOKEN_FUN;
        else if (identifier == "if")
            identifierType = TokenType::TOKEN_IF;
        else if (identifier == "nil")
            identifierType = TokenType::TOKEN_NIL;
        else if (identifier == "or")
            identifierType = TokenType::TOKEN_OR;
        // else if (identifier == "print")
        //     identifierType = TokenType::TOKEN_PRINT;
        else if (identifier == "return")
            identifierType = TokenType::TOKEN_RETURN;
        else if (identifier == "super")
            identifierType = TokenType::TOKEN_SUPER;
        else if (identifier == "this")
            identifierType = TokenType::TOKEN_THIS;
        else if (identifier == "true")
            identifierType = TokenType::TOKEN_TRUE;
        else if (identifier == "var")
            identifierType = TokenType::TOKEN_VAR;
        else if (identifier == "while")
            identifierType = TokenType::TOKEN_WHILE;

        return Token(identifierType, identifier, line, column);
    }

    Token Scanner::number()
    {
        while (isDigit(peek()))
            advance();

        // Look for a fractional part.
        if (peek() == '.' && isDigit(peek(1)))
        {
            // Consume the ".".
            advance();

            while (isDigit(peek()))
                advance();
        }

        return Token(TokenType::TOKEN_NUMBER, buffer.begin(), current, line, column);
    }

    void Scanner::skipWhitespace()
    {
        while (true)
        {
            char c = peek();
            switch (c)
            {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '\n':
                line++;
                column = 1;
                advance();
                break;
            case '/':
                if (peek(1) == '/')
                {
                    // A comment goes until the end of the line.
                    while (peek() != '\n' && !isAtEnd())
                        advance();
                }
                else
                {
                    return;
                }
                break;
            default:
                return;
            }
        }
    }

    char Scanner::advance()
    {
        if (isAtEnd())
            return '\0';

        char ch = *current;
        current++;
        column++;
        return ch;
    }
} // namespace lox
