
#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/AST/Expr.h"

#define ACCEPT_IMPL(name)                                                      \
  void name::accept(ASTVisitor &visitor) { visitor.visit(*this); }

namespace lox {

ACCEPT_IMPL(ThisExpr)
ACCEPT_IMPL(SuperExpr)
ACCEPT_IMPL(GroupingExpr)
ACCEPT_IMPL(CallExpr)
ACCEPT_IMPL(VariableExpr)
ACCEPT_IMPL(LiteralExpr)
ACCEPT_IMPL(NumberExpr)
ACCEPT_IMPL(StringExpr)
ACCEPT_IMPL(UnaryExpr)
ACCEPT_IMPL(BinaryExpr)
ACCEPT_IMPL(AccessExpr)
ACCEPT_IMPL(AssignExpr)

ACCEPT_IMPL(ExpressionStmt)
ACCEPT_IMPL(DeclarationStmt)
ACCEPT_IMPL(VarDeclStmt)
ACCEPT_IMPL(BlockStmt)
ACCEPT_IMPL(ClassDeclStmt)
ACCEPT_IMPL(FunctionDecl)
ACCEPT_IMPL(IfStmt)
ACCEPT_IMPL(WhileStmt)
ACCEPT_IMPL(ForStmt)
ACCEPT_IMPL(ReturnStmt)

} // namespace lox

#undef ACCEPT_IMPL
