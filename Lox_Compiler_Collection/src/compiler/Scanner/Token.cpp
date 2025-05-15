#include "Compiler/Scanner/TokenType.h"

namespace lox
{
    std::string convertTokenTypeToString(TokenType type) {
        switch (type) // Updated function name
        {
        case TOKEN_LEFT_PAREN: return "(";
        case TOKEN_RIGHT_PAREN: return ")";
        case TOKEN_LEFT_BRACE: return "{";
        case TOKEN_RIGHT_BRACE: return "}";
        case TOKEN_COMMA: return ",";
        case TOKEN_DOT: return ".";
        case TOKEN_MINUS: return "-";
        case TOKEN_PLUS: return "+";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_SLASH: return "/";
        case TOKEN_STAR: return "*";

        case TOKEN_BANG: return "!";
        case TOKEN_BANG_EQUAL: return "!=";
        case TOKEN_EQUAL: return "=";
        case TOKEN_EQUAL_EQUAL: return "==";
        case TOKEN_GREATER: return ">";
        case TOKEN_GREATER_EQUAL: return ">=";
        case TOKEN_LESS: return "<";
        case TOKEN_LESS_EQUAL: return "<=";

        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        // case TOKEN_STRING: return "TOKEN_STRING";
        case TOKEN_NUMBER: return "TOKEN_NUMBER";

        // Interpolation
        case TOKEN_INTERPOLATION_START: return "TOKEN_INTERPOLATION_START";
        case TOKEN_INTERPOLATION_END: return "TOKEN_INTERPOLATION_END";

        // Keywords.
        case TOKEN_AND: return "and";
        case TOKEN_CLASS: return "class";
        case TOKEN_ELSE: return "else";
        case TOKEN_FALSE: return "false";
        case TOKEN_FOR: return "for";
        case TOKEN_FUN: return "fun";
        case TOKEN_IF: return "if";
        case TOKEN_NIL: return "nil";
        case TOKEN_OR: return "or";
        // case TOKEN_PRINT: return "print";
        case TOKEN_RETURN: return "return";
        case TOKEN_SUPER: return "super";
        case TOKEN_THIS: return "this";
        case TOKEN_TRUE: return "true";
        case TOKEN_VAR: return "var";
        case TOKEN_WHILE: return "while";

        case TOKEN_EOF: return "[EOF]";

        default:
            return "<<unknown>>";
        }
    }
} // namespace lox
