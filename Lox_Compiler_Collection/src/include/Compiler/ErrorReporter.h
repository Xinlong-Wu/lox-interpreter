#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <iostream>

#include "Compiler/AST/Expr.h"

namespace lox {
class StmtBase;
class ErrorReporter {
private:
  static int errorCount;
  static int warningCount;

public:
  static void resetCounts();
  static int hasError();
  static int hasWarning();
  static int getErrorCount();
  static int getWarningCount();
  static void reportError(const StmtBase *stmt, const std::string &message,
                          std::ostream &os = std::cerr);
  static void reportWarning(const StmtBase *stmt, const std::string &message,
                            std::ostream &os = std::cerr);
  static void reportError(const ExprBase *expr, const std::string &message,
                          std::ostream &os = std::cerr);
  static void reportWarning(const ExprBase *expr, const std::string &message,
                            std::ostream &os = std::cerr);
  static void reportError(const std::string &message,
                          std::ostream &os = std::cerr);
  static void reportWarning(const std::string &message,
                            std::ostream &os = std::cerr);
};
} // namespace lox

#endif // ERROR_REPORTER_H
