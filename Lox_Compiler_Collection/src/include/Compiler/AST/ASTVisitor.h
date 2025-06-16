#ifndef ASTVISITOR_H
#define ASTVISITOR_H

namespace lox {
#define VISIT(name) virtual void visit(name &expr) = 0
#define INSTENCE_VISIT(name) void visit(name &expr) override
#define DEFINE_VISIT(klass, name) void lox::klass::visit(name &expr)

class NumberExpr;
class StringExpr;
class BoolExpr;
class NilExpr;
class VariableExpr;
class AccessExpr;
class UnaryExpr;
class BinaryExpr;
class AssignExpr;
class CallExpr;

class ExpressionStmt;
class VarDeclStmt;
class BlockStmt;
class ClassDeclStmt;
class FunctionDeclStmt;
class IfStmt;
class WhileStmt;
class ForStmt;
class ReturnStmt;
class BreakStmt;
class ContinueStmt;

class ASTVisitor {
public:
  virtual ~ASTVisitor() = default;

  VISIT(NumberExpr);
  VISIT(StringExpr);
  VISIT(BoolExpr);
  VISIT(NilExpr);
  VISIT(VariableExpr);
  VISIT(AccessExpr);
  VISIT(UnaryExpr);
  VISIT(BinaryExpr);
  VISIT(AssignExpr);
  VISIT(CallExpr);

  VISIT(ExpressionStmt);
  VISIT(VarDeclStmt);
  VISIT(BlockStmt);
  VISIT(ClassDeclStmt);
  VISIT(FunctionDeclStmt);
  VISIT(IfStmt);
  VISIT(WhileStmt);
  VISIT(ForStmt);
  VISIT(ReturnStmt);
  VISIT(BreakStmt);
  VISIT(ContinueStmt);
};

#undef VISIT
} // namespace lox

#endif // ASTVISITOR_H