#ifndef LOX_COMPILER_TOKEN_H
#define LOX_COMPILER_TOKEN_H

#include "Compiler/Enum.h"
#include "Compiler/Location.h"

#include <cassert>
#include <memory>
#include <optional>
#include <string>

namespace lox {
class TokenError;
class Token {
public:
  Token() = default;
  Token(TokenType tokenType)
      : Token(tokenType, std::string_view(), Location(-1, -1)){};
  Token(std::string_view tokenStr)
      : Token(TokenType::TOKEN_UNKNOWN, tokenStr, Location(-1, -1)){};
  Token(TokenType tokenType, std::string_view tokenStr, int line, int column)
      : Token(tokenType, tokenStr, Location(line, column)){};
  Token(TokenType tokenType, const char *start, int length, Location location)
      : Token(tokenType, std::string_view(start, length), location){};
  Token(TokenType tokenType, const char *start, const char *end,
        Location location)
      : Token(tokenType, start, end - start, location){};
  Token(TokenType tokenType, const char *start, const char *end, int line,
        int column)
      : Token(tokenType, start, end - start, Location(line, column)){};
  Token(TokenType tokenType, const char *start, int length, int line,
        int column)
      : Token(tokenType, std::string_view(start, length),
              Location(line, column)){};
  Token(TokenType tokenType, std::string_view tokenStr, Location location)
      : Token(tokenType, tokenStr, location, std::nullopt){};
  Token(const Token &token)
      : Token(token.type, token.tokenStr, token.location, token.errorMsg){};
  virtual ~Token() =
      default; // Add a virtual destructor to make Token polymorphic

  size_t length() const { return tokenStr.length(); }

  bool operator==(lox::TokenType tokenType) const { return type == tokenType; }

  bool operator==(std::string str) const { return tokenStr.compare(str) == 0; }

  bool operator!=(lox::TokenType tokenType) const { return type != tokenType; }

  bool operator!=(const lox::Token &token) const {
    return type != token.type || tokenStr.compare(token.tokenStr) != 0 ||
           location != token.location;
  }

  const std::string_view &getTokenString() { return tokenStr; }

  TokenType getType() const { return type; }

  Location getLoction() const { return location; }

  friend std::ostream &operator<<(std::ostream &os, const Token &token) {
    token.print(os);
    return os;
  }

  void print(std::ostream &os = std::cout) const {
    if (!tokenStr.empty()) {
      os << "`" << tokenStr << "`";
    } else {
      os << convertTokenTypeToString(type);
    }
  }

protected:
  TokenType type = TokenType::TOKEN_UNINITIALIZED;
  std::string_view tokenStr;
  Location location;

  std::optional<std::string> errorMsg;

  Token(TokenType tokenType, std::string_view tokenStr, Location loc,
        const std::optional<std::string> &errMsg)
      : type(tokenType), tokenStr(tokenStr), location(loc), errorMsg(errMsg) {
    assert(tokenType != TokenType::TOKEN_ERROR ||
           (tokenType == TokenType::TOKEN_ERROR && errMsg.has_value()));
  };
};

class TokenError : public Token {
public:
  TokenError(const std::string &msg) : TokenError(msg, -1, -1){};
  TokenError(const std::string &msg, int line, int column)
      : TokenError("", msg, Location(line, column)){};
  TokenError(const std::string_view tokenStr,
             const std::optional<std::string> &errorMsg, Location loc)
      : Token(TokenType::TOKEN_ERROR, tokenStr, loc, errorMsg){};
  TokenError(const Token &token) : Token(token) {
    assert(type == TokenType::TOKEN_ERROR);
  };
  TokenError(const Token &token, const std::string &errorMsg) : Token(token) {
    type = TokenType::TOKEN_ERROR;
    this->errorMsg = errorMsg;
  };
  ~TokenError(){};

  std::string getErrorMsg() const { return errorMsg.value_or("Unknown error"); }

  bool hasErrorMsg() const { return errorMsg.has_value(); }

  void setErrorMsg(const std::string &msg) { errorMsg = msg; }
};
} // namespace lox

#endif // LOX_COMPILER_TOKEN_H
