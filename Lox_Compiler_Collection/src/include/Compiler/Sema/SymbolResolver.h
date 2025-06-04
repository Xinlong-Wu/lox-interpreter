
#ifndef SYMBOLRESOLVER_H
#define SYMBOLRESOLVER_H

#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/Sema/SymbolTable.h"

namespace lox {

class SymbolResolver : public ASTVisitor {
private:
  SymbolTable symbolTable;

  void inilializeGlobalScope() {
    symbolTable.declare(std::make_shared<
                        Symbol>(std::shared_ptr<FunctionType>(new FunctionType(
        "print",
        {std::shared_ptr<FunctionType::Signature>(new FunctionType::Signature(
             {StringType::getInstance()}, NilType::getInstance())),
         std::shared_ptr<FunctionType::Signature>(new FunctionType::Signature(
             {NumberType::getInstance()}, NilType::getInstance())),
         std::shared_ptr<FunctionType::Signature>(new FunctionType::Signature(
             {BoolType::getInstance()}, NilType::getInstance()))}))));
  }
public:
  SymbolResolver() {
    // Initialize the global scope with built-in functions
    inilializeGlobalScope();
  }
  ~SymbolResolver() override = default;

  void resolve(std::vector<std::unique_ptr<StmtBase>> &statements) {
    symbolTable.enterScope();

    for (auto &stmt : statements) {
      stmt->accept(*this);
    }

    symbolTable.exitScope();
  }

  INSTENCE_VISIT(ThisExpr);
  INSTENCE_VISIT(SuperExpr);
  INSTENCE_VISIT(GroupingExpr);
  INSTENCE_VISIT(CallExpr);
  INSTENCE_VISIT(VariableExpr);
  INSTENCE_VISIT(LiteralExpr);
  INSTENCE_VISIT(NumberExpr);
  INSTENCE_VISIT(StringExpr);
  INSTENCE_VISIT(UnaryExpr);
  INSTENCE_VISIT(BinaryExpr);
  INSTENCE_VISIT(AccessExpr);
  INSTENCE_VISIT(AssignExpr);

  INSTENCE_VISIT(ExpressionStmt);
  INSTENCE_VISIT(DeclarationStmt);
  INSTENCE_VISIT(VarDeclStmt);
  INSTENCE_VISIT(BlockStmt);
  INSTENCE_VISIT(ClassDeclStmt);
  INSTENCE_VISIT(FunctionDecl);
  INSTENCE_VISIT(IfStmt);
  INSTENCE_VISIT(WhileStmt);
  INSTENCE_VISIT(ForStmt);
  INSTENCE_VISIT(ReturnStmt);
};

} // namespace lox

#endif // SYMBOLRESOLVER_H