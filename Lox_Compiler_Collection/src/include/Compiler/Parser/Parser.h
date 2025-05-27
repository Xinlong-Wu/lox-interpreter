
#ifndef PARSER_H
#define PARSER_H

#include "Compiler/AST/Expr.h"
#include "Compiler/AST/Stmt.h"
#include "Compiler/Scanner/Scanner.h"
#include "Compiler/Scanner/Token.h"

namespace lox {
class Parser {
private:
  Scanner scanner;
  Token currentToken;
  Token previousToken;
  bool error = false;
  bool panicMode = false;

  std::unique_ptr<ExprBase> parseExpression();
  std::unique_ptr<StmtBase> parseStatement();
  std::unique_ptr<ExpressionStmt> parseExpressionStmt();
  std::unique_ptr<BlockStmt> parseBlockStmt();
  std::unique_ptr<IfStmt> parseIfStmt();
  std::unique_ptr<ReturnStmt> parseReturnStmt();
  std::unique_ptr<ForStmt> parseForStmt();
  std::unique_ptr<WhileStmt> parseWhileStmt();

  std::unique_ptr<VarDeclStmt> parseVarDecl();
  std::unique_ptr<FunctionDecl> parseFunctionDecl();
  std::unique_ptr<ClassDeclStmt> parseClassDecl();

public:
  Parser(const char *source) : scanner(source){};
  Parser(Scanner scanner) : scanner(scanner){};
  ~Parser(){};

  std::unique_ptr<StmtBase> parseDeclaration();

  Token &getCurrentToken() { return currentToken; };
  Token &getPreviousToken() { return previousToken; };

  void advance();
  bool match(TokenType type) { return currentToken == type; };
  bool hasNext() { return !match(lox::TokenType::TOKEN_EOF); };
  bool hasError() { return error; };
  bool isPanicMode() { return panicMode; };
  void synchronize();
  void synchronize(TokenType tokenType);

  void parse(TokenType type,
             std::optional<std::string> errorMsg = std::nullopt);
  bool parseOptional(TokenType type);
  void parseError(TokenType tokenType, bool shouldPanic = true);
  void parseError(const Token &token, bool shouldPanic = true);
  void parseError(std::string_view message, bool shouldPanic = true);
  void parseError(const std::unique_ptr<ExprBase> &expr,
                  std::string_view message, bool shouldPanic = true);
};
} // namespace lox

#endif // PARSER_H
