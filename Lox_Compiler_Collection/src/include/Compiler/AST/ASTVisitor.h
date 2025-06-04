#ifndef ASTVISITOR_H
#define ASTVISITOR_H
#include "Compiler/AST/Expr.h"
#include "Compiler/AST/Stmt.h"

namespace lox {
#define VISIT(name) virtual void visit(name &expr) = 0
#define INSTENCE_VISIT(name) void visit(name &expr) override
#define DEFINE_VISIT(klass, name) void lox::klass::visit(name &expr)

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  VISIT(ThisExpr);
  VISIT(SuperExpr);
  VISIT(GroupingExpr);
  VISIT(CallExpr);
  VISIT(VariableExpr);
  VISIT(LiteralExpr);
  VISIT(NumberExpr);
  VISIT(StringExpr);
  VISIT(UnaryExpr);
  VISIT(BinaryExpr);
  VISIT(AccessExpr);
  VISIT(AssignExpr);

  VISIT(ExpressionStmt);
  VISIT(DeclarationStmt);
  VISIT(VarDeclStmt);
  VISIT(BlockStmt);
  VISIT(ClassDeclStmt);
  VISIT(FunctionDecl);
  VISIT(IfStmt);
  VISIT(WhileStmt);
  VISIT(ForStmt);
  VISIT(ReturnStmt);
};

#undef VISIT
} // namespace lox

#endif // ASTVISITOR_H