#include "compiler/Parser.h"
#include "compiler/Token.h"
#include <unordered_map>

namespace
{
    using lox::ExprBase;
    using lox::Parser;
    using lox::TokenType;
    using PrefixFn = std::unique_ptr<ExprBase> (*)(Parser&);
    using InfixFn = std::unique_ptr<ExprBase> (*)(Parser&, std::unique_ptr<ExprBase>);

    #define PrefixHandler(name) static std::unique_ptr<ExprBase> name(Parser& parser)
    #define InfixHandler(name) static std::unique_ptr<ExprBase> name(Parser& parser, std::unique_ptr<ExprBase> left)

    enum Precedence {
        PREC_NONE,
        PREC_THIS,
        PREC_SUPER,
        PREC_ASSIGNMENT,
        PREC_OR,
        PREC_AND,
        PREC_EQUALITY,
        PREC_COMPARISON,
        PREC_TERM,
        PREC_FACTOR,
        PREC_UNARY,
        PREC_CALL,
        PREC_PRIMARY
    };

    struct ParseRule {
        PrefixFn prefix;
        InfixFn infix;
        Precedence precedence;
    };

    extern std::unordered_map<lox::TokenType, ParseRule> rules;

    static std::unique_ptr<ExprBase> parsePrecedence(Parser &parser, Precedence precedence) {
        parser.advance();
        PrefixFn prefixRule = rules[parser.getPreviousToken().getType()].prefix;
        if (prefixRule == NULL) {
            parser.parseError("Expect expression.");
            return nullptr;
        }

        bool canAssign = precedence <= PREC_ASSIGNMENT;
        // prefixRule(canAssign);
        std::unique_ptr<ExprBase> left = prefixRule(parser);

        while (precedence <= rules[parser.getCurrentToken().getType()].precedence) {
            parser.advance();
            InfixFn infixRule = rules[parser.getPreviousToken().getType()].infix;
        //   infixRule(canAssign);
            left = infixRule(parser, std::move(left));
        }

        if (canAssign && parser.parseOptional(lox::TokenType::TOKEN_EQUAL)) {
          parser.parseError("Invalid assignment target.");
        }

        return left;
    }

    PrefixHandler(unary) {
        TokenType operatorType = parser.getPreviousToken().getType();

        // Compile the operand.
        std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_UNARY);

        // Emit the operator instruction.
        switch (operatorType) {
        case lox::TokenType::TOKEN_BANG:
        case lox::TokenType::TOKEN_MINUS:
            return std::make_unique<lox::UnaryExpr>(operatorType, std::move(right));
        default:
            parser.parseError("Invalid unary operator.");
            return nullptr;
        }
    }

    PrefixHandler(number) {
        return std::make_unique<lox::NumberExpr>(parser.getPreviousToken());
    }

    PrefixHandler(literal) {
        switch (parser.getPreviousToken().getType()) {
        case lox::TokenType::TOKEN_FALSE:
        case lox::TokenType::TOKEN_NIL:
        case lox::TokenType::TOKEN_TRUE:
            return std::make_unique<lox::LiteralExpr>(parser.getPreviousToken());
        default:
            parser.parseError("Invalid literal.");
            return nullptr;
        }
    }

    PrefixHandler(parseString) {
        return std::make_unique<lox::StringExpr>(parser.getPreviousToken());
    }

    PrefixHandler(variable) {
        return std::make_unique<lox::VariableExpr>(parser.getPreviousToken());
    }

    PrefixHandler(this_) {
        return std::make_unique<lox::ThisExpr>(parser.getPreviousToken());
    }

    PrefixHandler(super_) {
        return std::make_unique<lox::SuperExpr>(parser.getPreviousToken());
    }

    PrefixHandler(grouping) {
        // Compile the inner expression.
        std::unique_ptr<ExprBase> expr = parser.parseExpression();

        // Consume the ")"
        parser.parse(lox::TokenType::TOKEN_RIGHT_PAREN);

        return std::make_unique<lox::GroupingExpr>(std::move(expr));
    }

    InfixHandler(or_) {
        // Compile the right operand.
        std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_OR);

        // Emit the operator instruction.
        return std::make_unique<lox::BinaryExpr>(lox::TokenType::TOKEN_OR, std::move(left), std::move(right));
    }

    InfixHandler(and_) {
        // Compile the right operand.
        std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_AND);

        // Emit the operator instruction.
        return std::make_unique<lox::BinaryExpr>(lox::TokenType::TOKEN_AND, std::move(left), std::move(right));
    }

    InfixHandler(dot) {
        parser.parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect property name after '.'.");
        // uint8_t name = parser.identifierConstant(parser.getPreviousToken());
        lox::Token symName = parser.getPreviousToken();

        return std::make_unique<lox::AccessExpr>(std::move(left), symName);
    }

    InfixHandler(assign) {
        // Compile the right operand.
        std::unique_ptr<ExprBase> value = parsePrecedence(parser, PREC_ASSIGNMENT);

        if (!lox::AssignExpr::isValidLValue(left)) {
            parser.parseError(left, "Invalid left Value");
            return nullptr;
        }

        // Emit the operator instruction.
        return std::make_unique<lox::AssignExpr>(std::move(left), std::move(value));
    }

    std::vector<std::unique_ptr<ExprBase>> argumentList(Parser &parser) {
        std::vector<std::unique_ptr<ExprBase>> args;
        if (!parser.match(lox::TokenType::TOKEN_RIGHT_PAREN)) {
            do {
                args.push_back(parser.parseExpression());
            } while (parser.parseOptional(lox::TokenType::TOKEN_COMMA));
        }
        return args;
    }

    InfixHandler(call) {
        std::vector<std::unique_ptr<ExprBase>> args = {};
        if (!parser.parseOptional(lox::TokenType::TOKEN_RIGHT_PAREN)) {
            args = argumentList(parser);
            parser.parse(lox::TokenType::TOKEN_RIGHT_PAREN);
        }

        return std::make_unique<lox::CallExpr>(std::move(left), std::move(args));
    }

    InfixHandler(binary) {
        TokenType operatorType = parser.getPreviousToken().getType();
        Precedence precedence = (Precedence)(rules[operatorType].precedence + 1);

        // Compile the right operand.
        std::unique_ptr<ExprBase> right = parsePrecedence(parser, precedence);

        // Emit the operator instruction.
        switch (operatorType) {
        case lox::TokenType::TOKEN_BANG_EQUAL:
        case lox::TokenType::TOKEN_EQUAL_EQUAL:
        case lox::TokenType::TOKEN_GREATER:
        case lox::TokenType::TOKEN_GREATER_EQUAL:
        case lox::TokenType::TOKEN_LESS:
        case lox::TokenType::TOKEN_LESS_EQUAL:
        case lox::TokenType::TOKEN_PLUS:
        case lox::TokenType::TOKEN_MINUS:
        case lox::TokenType::TOKEN_SLASH:
        case lox::TokenType::TOKEN_STAR:
            return std::make_unique<lox::BinaryExpr>(operatorType, std::move(left), std::move(right));
        default:
            parser.parseError("Invalid binary operator.");
            return nullptr;
        }
    }

    static std::unordered_map<lox::TokenType, ParseRule> rules = {
        {lox::TokenType::TOKEN_LEFT_PAREN,    {grouping, call,   PREC_CALL}},
        {lox::TokenType::TOKEN_RIGHT_PAREN,   {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_LEFT_BRACE,    {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_RIGHT_BRACE,   {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_COMMA,         {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_DOT,           {NULL,     dot,    PREC_CALL}},
        {lox::TokenType::TOKEN_MINUS,         {unary,    binary, PREC_TERM}},
        {lox::TokenType::TOKEN_PLUS,          {NULL,     binary, PREC_TERM}},
        {lox::TokenType::TOKEN_SEMICOLON,     {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_SLASH,         {NULL,     binary, PREC_FACTOR}},
        {lox::TokenType::TOKEN_STAR,          {NULL,     binary, PREC_FACTOR}},
        {lox::TokenType::TOKEN_BANG,          {unary,    NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_EQUAL,         {NULL,     assign, PREC_ASSIGNMENT}},
        {lox::TokenType::TOKEN_BANG_EQUAL,    {NULL,     binary, PREC_EQUALITY}},
        {lox::TokenType::TOKEN_EQUAL_EQUAL,   {NULL,     binary, PREC_EQUALITY}},
        {lox::TokenType::TOKEN_GREATER,       {NULL,     binary, PREC_COMPARISON}},
        {lox::TokenType::TOKEN_GREATER_EQUAL, {NULL,     binary, PREC_COMPARISON}},
        {lox::TokenType::TOKEN_LESS,          {NULL,     binary, PREC_COMPARISON}},
        {lox::TokenType::TOKEN_LESS_EQUAL,    {NULL,     binary, PREC_COMPARISON}},
        {lox::TokenType::TOKEN_IDENTIFIER,    {variable, NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_STRING,        {parseString,   NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_NUMBER,        {number,   NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_AND,           {NULL,     and_,   PREC_AND}},
        {lox::TokenType::TOKEN_CLASS,         {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_ELSE,          {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_FALSE,         {literal,  NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_FOR,           {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_FUN,           {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_IF,            {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_NIL,           {literal,  NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_OR,            {NULL,     or_,    PREC_OR}},
        {lox::TokenType::TOKEN_PRINT,         {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_RETURN,        {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_SUPER,         {super_,   NULL,   PREC_SUPER}},
        {lox::TokenType::TOKEN_THIS,          {this_,    NULL,   PREC_THIS}},
        {lox::TokenType::TOKEN_TRUE,          {literal,  NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_VAR,           {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_WHILE,         {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_ERROR,         {NULL,     NULL,   PREC_NONE}},
        {lox::TokenType::TOKEN_EOF,           {NULL,     NULL,   PREC_NONE}},
    };
} // namespace

namespace lox
{
    std::unique_ptr<ExprBase> Parser::parseExpression()
    {
        return parsePrecedence(*this, PREC_ASSIGNMENT);
    }
} // namespace lox
