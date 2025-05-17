#ifndef ENUM_H
#define ENUM_H

#include <string>

namespace lox
{
    enum Type {
        TYPE_UNKNOWN,
        TYPE_NUMBER,
        TYPE_STRING,
        TYPE_BOOL,
        TYPE_NIL,
        TYPE_OBJECT,
        TYPE_FUNCTION,
        TYPE_CLASS,
        TYPE_INSTANCE
    };

    static std::string convertTypeToString(Type type) {
        switch (type) {
            case TYPE_UNKNOWN: return "unknown";
            case TYPE_NUMBER: return "number";
            case TYPE_STRING: return "string";
            case TYPE_BOOL: return "bool";
            case TYPE_NIL: return "nil";
            case TYPE_OBJECT: return "object";
            case TYPE_FUNCTION: return "function";
            case TYPE_CLASS: return "class";
            case TYPE_INSTANCE: return "instance";
            default: return "unknown";
        }
    }

    enum TokenType {
        // Single-character tokens.
        TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
        TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
        TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
        TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
        // One or two character tokens.
        TOKEN_BANG, TOKEN_BANG_EQUAL,
        TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
        TOKEN_GREATER, TOKEN_GREATER_EQUAL,
        TOKEN_LESS, TOKEN_LESS_EQUAL,
        // Literals.
        TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

        // Interpolation
        TOKEN_INTERPOLATION_START, TOKEN_INTERPOLATION_END,

        // Keywords.
        TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
        TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
        /* TOKEN_PRINT ,*/ TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
        TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

        TOKEN_ERROR, TOKEN_EOF, TOKEN_UNKNOWN,

        TOKEN_UNINITIALIZED
    };

    static std::string convertTokenTypeToString(TokenType type) {
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


#endif // ENUM_H