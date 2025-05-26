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
    Sema() {
        // Initialize the global scope with built-in functions
        inilializeGlobalScope();
    };
    ~Sema() override = default;

    void inilializeGlobalScope() {
        symbolTable.declare(std::make_shared<Symbol>(std::make_shared<FunctionType>("print", std::vector<std::shared_ptr<Type>>{StringType::getInstance()}, NilType::getInstance())));
    }

    void enterScope(const std::string& name = "anonymous") {
        symbolTable.enterScope(name);
    }
    void exitScope() {
        symbolTable.exitScope();
    }

    void analyze(std::vector<std::unique_ptr<StmtBase>>& statements) {
        symbolTable.enterScope();

        for (auto& stmt : statements) {
            stmt->accept(*this);
        }

        symbolTable.exitScope();
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