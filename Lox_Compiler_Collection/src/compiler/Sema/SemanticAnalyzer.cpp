#include "Compiler/Sema/SemanticAnalyzer.h"
#include "Compiler/Sema/TypeSystem/TypeInfer/TypeInferenceEngine.h"

using namespace lox;
using namespace std;

void Sema::analyze(TypeContext &typeContext, const std::vector<std::unique_ptr<lox::StmtBase>>& statements) {
  // TypeInfer
  TypeInferenceEngine typeInferEngine(&typeContext);
  typeInferEngine.inferProgramTypes(statements);
}
