#ifndef TYPE_INFERENCE_ENGINE_H
#define TYPE_INFERENCE_ENGINE_H

#include "Compiler/AST/Expr.h"
#include "Compiler/AST/Stmt.h"
#include "Compiler/Sema/SymbolTable.h"
#include "Compiler/Sema/TypeSystem/Type.h"
#include "Compiler/Sema/TypeSystem/TypeInfer/Constraint.h"

namespace lox {
class TypeInferenceEngine {
private:
  TypeContext *typeContext;
  SymbolTable symbolTable;
  std::vector<Constraint> constraints;
  std::unordered_map<const TypeVariable *, Type *> substitutions;

  void collectTypeDeclarations(
      const std::vector<std::unique_ptr<StmtBase>> &statements);
  void collectClassDeclarations(ClassDeclStmt *classDecl);
  void collectFunctionDeclarations(FunctionDeclStmt *funcDecl);

  void
  inferStatements(const std::vector<std::unique_ptr<StmtBase>> &statements);
  void inferStatement(StmtBase *stmt);
  void inferVarDeclStmt(VarDeclStmt *varDecl);
  void inferFunctionDeclStmt(FunctionDeclStmt *funcDecl);
  void inferReturnStmt(ReturnStmt *returnStmt);
  void inferClassDeclStmt(ClassDeclStmt *classDecl);
  void inferBlockStmt(BlockStmt *blockStmt);
  void inferExprStmt(ExpressionStmt *exprStmt);

  Type *inferExpr(ExprBase *expr, Type *expectedType = nullptr);
  Type *inferBinaryExpr(BinaryExpr *binaryExpr, Type *expectedType = nullptr);
  Type *inferUnaryExpr(UnaryExpr *unaryExpr,
                       const Type *expectedType = nullptr);
  Type *inferCallExpr(CallExpr *callExpr, Type *expectedType = nullptr);
  Type *inferAssignExpr(AssignExpr *assignExpr, Type *expectedType = nullptr);
  Type *inferAccessExpr(AccessExpr *accessExpr,
                        const Type *expectedType = nullptr);

  bool solveConstraints();
  bool solveConstraint(Type *left, Type *right,
                       Constraint::ConstraintType relation);
  bool unify(Type *left, Type *right);
  // check if left is assignable to right
  bool assinable(Type *left, Type *right);
  // to avoid infinite recursion, we need to check if a type variable occurs in
  // a type e.g. T occurs in T -> false, T occurs in T -> true
  bool occursCheck(const TypeVariable *var, Type *type);

  Type *applySubstitution(Type *type);
  void
  applySubstitutions(const std::vector<std::unique_ptr<StmtBase>> &statements);

public:
  TypeInferenceEngine(TypeContext *typeContext);
  ~TypeInferenceEngine() = default;

  // 添加约束
  void addConstraint(Type *left, Type *right,
                     Constraint::ConstraintType relation) {
    constraints.emplace_back(left, right, relation);
    solveConstraint(left, right, relation);
  }

  void
  inferProgramTypes(const std::vector<std::unique_ptr<StmtBase>> &statements);

  void printConstraints() const;
};
} // namespace lox

#endif // TYPE_INFERENCE_ENGINE_H
