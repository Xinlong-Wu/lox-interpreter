#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/ErrorReporter.h"

namespace lox
{

class Sema : public ASTVisitor {
private:
    SymbolTable symbolTable;

public:
    Sema() = default;
    ~Sema() override = default;

    void analyze(std::unique_ptr<StmtBase>& stmt) {
        stmt->accept(*this);
    }

    #define INSTENCE_VISIT(name) \
        void visit(name& expr) override

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

    #undef INSTENCE_VISIT
};

} // namespace lox

#endif // SEMANTICANALYZER_H