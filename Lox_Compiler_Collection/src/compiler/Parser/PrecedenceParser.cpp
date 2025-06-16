#include "Common.h"
#include "Compiler/Parser/Parser.h"
#include "Compiler/Scanner/Token.h"
#include <unordered_map>

namespace lox {
using PrefixFn = std::unique_ptr<ExprBase> (*)(Parser &);
using InfixFn = std::unique_ptr<ExprBase> (*)(Parser &,
                                              std::unique_ptr<ExprBase>);

#define PrefixHandler(name) std::unique_ptr<ExprBase> name(Parser &parser)
#define InfixHandler(name)                                                     \
  std::unique_ptr<ExprBase> name(Parser &parser, std::unique_ptr<ExprBase> left)

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

static std::unique_ptr<ExprBase> parsePrecedence(Parser &parser,
                                                 Precedence precedence) {
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

std::unique_ptr<ExprBase> expression(Parser &parser) {
  return parsePrecedence(parser, PREC_ASSIGNMENT);
}

PrefixHandler(unary) {
  TokenType operatorType = parser.getPreviousToken().getType();

  // Compile the operand.
  std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
  case lox::TokenType::TOKEN_BANG:
    return std::make_unique<lox::UnaryExpr>(lox::UnaryExpr::Op::Not, std::move(right),
                                            parser.getPreviousToken().getLoction());
  case lox::TokenType::TOKEN_MINUS:
    return std::make_unique<lox::UnaryExpr>(lox::UnaryExpr::Op::Negate, std::move(right),
                                            parser.getPreviousToken().getLoction());
  default:
    parser.parseError("Invalid unary operator.");
    return nullptr;
  }
}

PrefixHandler(number) {
  return std::make_unique<lox::NumberExpr>(std::stod(std::string(parser.getPreviousToken().getTokenString())),
                                           parser.getPreviousToken().getLoction());
}

PrefixHandler(literal) {
  switch (parser.getPreviousToken().getType()) {
  case lox::TokenType::TOKEN_FALSE:
    return std::make_unique<lox::BoolExpr>(false, parser.getPreviousToken().getLoction());
  case lox::TokenType::TOKEN_TRUE:
    return std::make_unique<lox::BoolExpr>(true, parser.getPreviousToken().getLoction());
  case lox::TokenType::TOKEN_NIL:
    return std::make_unique<lox::NilExpr>(parser.getPreviousToken().getLoction());
  default:
    parser.parseError("Invalid literal.");
    return nullptr;
  }
}

PrefixHandler(parseString) {
  return std::make_unique<lox::StringExpr>(
      parser.getPreviousToken().getTokenString().substr(1, // Skip the opening quote
                                              parser.getPreviousToken().getTokenString().size() - 2),
      parser.getPreviousToken().getLoction());
}

PrefixHandler(variable) {
  return std::make_unique<lox::VariableExpr>(
      parser.getPreviousToken().getTokenString(),
      parser.getPreviousToken().getLoction());
}

PrefixHandler(this_) {
  return std::make_unique<lox::VariableExpr>(
      parser.getPreviousToken().getTokenString(),
      parser.getPreviousToken().getLoction());
}

PrefixHandler(super_) {
  return std::make_unique<lox::VariableExpr>(
      parser.getPreviousToken().getTokenString(),
      parser.getPreviousToken().getLoction());
}

PrefixHandler(grouping) {
  // Compile the inner expression.
  std::unique_ptr<ExprBase> expr = expression(parser);

  // Consume the ")"
  parser.parse(lox::TokenType::TOKEN_RIGHT_PAREN);

  return expr;
}

InfixHandler(or_) {
  Location loc = parser.getPreviousToken().getLoction();

  // Compile the right operand.
  std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_OR);

  // Emit the operator instruction.
  return std::make_unique<lox::BinaryExpr>(lox::BinaryExpr::Op::Or,
                                           std::move(left), std::move(right), loc);
}

InfixHandler(and_) {

  Location loc = parser.getPreviousToken().getLoction();

  // Compile the right operand.
  std::unique_ptr<ExprBase> right = parsePrecedence(parser, PREC_AND);

  // Emit the operator instruction.
  return std::make_unique<lox::BinaryExpr>(lox::BinaryExpr::Op::And,
                                           std::move(left), std::move(right), loc);
}

InfixHandler(dot) {
  Location loc = parser.getPreviousToken().getLoction();

  parser.parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect property name");
  if (parser.getPreviousToken() != lox::TokenType::TOKEN_IDENTIFIER) {
    return left;
  }

  // uint8_t name = parser.identifierConstant(parser.getPreviousToken());
  std::string propertyName = std::string(parser.getPreviousToken().getTokenString());

  return std::make_unique<lox::AccessExpr>(std::move(left), propertyName, loc);
}

InfixHandler(assign) {
  Location loc = parser.getPreviousToken().getLoction();
  // Compile the right operand.
  std::unique_ptr<ExprBase> value = parsePrecedence(parser, PREC_ASSIGNMENT);

  // Emit the operator instruction.
  return std::make_unique<lox::AssignExpr>(std::move(left), std::move(value), loc);
}

std::vector<std::unique_ptr<ExprBase>> argumentList(Parser &parser) {
  std::vector<std::unique_ptr<ExprBase>> args;
  if (!parser.match(lox::TokenType::TOKEN_RIGHT_PAREN)) {
    do {
      args.push_back(expression(parser));
    } while (parser.parseOptional(lox::TokenType::TOKEN_COMMA));
  }
  return args;
}

InfixHandler(call) {
  Location loc = parser.getPreviousToken().getLoction();
  std::vector<std::unique_ptr<ExprBase>> args = {};
  if (!parser.parseOptional(lox::TokenType::TOKEN_RIGHT_PAREN)) {
    args = argumentList(parser);
    parser.parse(lox::TokenType::TOKEN_RIGHT_PAREN);
  }

  assert ((isa<VariableExpr, AccessExpr>(left)) &&
         "Call expression must have a variable or access expression as the callee");
  // If the callee is not a variable, we cannot create a CallExpr.
  return std::make_unique<lox::CallExpr>(std::move(left), std::move(args),
                                         loc);
}

InfixHandler(binary) {
  TokenType operatorType = parser.getPreviousToken().getType();
  Precedence precedence = (Precedence)(rules[operatorType].precedence + 1);

  Location loc = parser.getPreviousToken().getLoction();

  // Compile the right operand.
  std::unique_ptr<ExprBase> right = parsePrecedence(parser, precedence);

  // Emit the operator instruction.
  lox::BinaryExpr::Op op;
  switch (operatorType) {
  case lox::TokenType::TOKEN_BANG_EQUAL:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::NotEqual, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_EQUAL_EQUAL:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::Equal, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_GREATER:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::GreaterThan, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_GREATER_EQUAL:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::GreaterThanEqual, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_LESS:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::GreaterThanEqual, std::move(right), std::move(left), loc);
  case lox::TokenType::TOKEN_LESS_EQUAL:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::GreaterThan, std::move(right), std::move(left), loc);
  case lox::TokenType::TOKEN_PLUS:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::Add, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_MINUS:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::Sub, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_SLASH:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::Div, std::move(left), std::move(right), loc);
  case lox::TokenType::TOKEN_STAR:
    return std::make_unique<lox::BinaryExpr>(
        lox::BinaryExpr::Op::Mul, std::move(left), std::move(right), loc);
  default:
    parser.parseError("Invalid binary operator.");
    return nullptr;
  }
}

std::unordered_map<lox::TokenType, ParseRule> rules = {
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
    // {lox::TokenType::TOKEN_PRINT,         {NULL,     NULL,   PREC_NONE}},
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

namespace lox {
std::unique_ptr<ExprBase> Parser::parseExpression() {
  return expression(*this);
}
} // namespace lox
