#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include "Compiler/AST/Stmt.h"
#include <memory>
#include <vector>

namespace lox {

class Sema {
private:

public:
  Sema(){};
  ~Sema() = default;

  static void
  analyze(TypeContext &typeContext,
          const std::vector<std::unique_ptr<lox::StmtBase>> &statements);
};

} // namespace lox

#endif // SEMANTICANALYZER_H