#include "Compiler/ErrorReporter.h"

#include "Compiler/AST/Expr.h"
#include "Compiler/AST/Stmt.h"
namespace lox {
int ErrorReporter::errorCount = 0;
int ErrorReporter::warningCount = 0;

void ErrorReporter::resetCounts() {
  errorCount = 0;
  warningCount = 0;
}

int ErrorReporter::hasError() { return errorCount > 0; }

int ErrorReporter::hasWarning() { return warningCount > 0; }
int ErrorReporter::getErrorCount() { return errorCount; }
int ErrorReporter::getWarningCount() { return warningCount; }
void ErrorReporter::reportError(const StmtBase *stmt,
                                const std::string &message, std::ostream &os) {
  os << "Error: " << message << " at " << stmt->getLoc() << std::endl;
  errorCount++;
}
void ErrorReporter::reportWarning(const StmtBase *stmt,
                                  const std::string &message,
                                  std::ostream &os) {
  os << "Warning: " << message << " at " << stmt->getLoc() << std::endl;
  warningCount++;
}

void ErrorReporter::reportError(const ExprBase *expr,
                                const std::string &message, std::ostream &os) {
  os << "Error: " << message << " at " << expr->getLoc() << std::endl << "\t ";
  expr->print(os);
  os << std::endl;
  errorCount++;
}
void ErrorReporter::reportWarning(const ExprBase *expr,
                                  const std::string &message,
                                  std::ostream &os) {
  os << "Warning: " << message << " at " << expr->getLoc() << std::endl
     << "\t ";
  expr->print(os);
  os << std::endl;
  warningCount++;
}

void ErrorReporter::reportError(const std::string &message, std::ostream &os) {
  os << "Error: " << message << std::endl;
  errorCount++;
}
void ErrorReporter::reportWarning(const std::string &message,
                                  std::ostream &os) {
  os << "Warning: " << message << std::endl;
  warningCount++;
}
} // namespace lox
