#ifndef SEMANTICANALYZER_H
#define SEMANTICANALYZER_H

#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/ErrorReporter.h"

namespace lox
{

struct SemanticContext {
    bool insideClass = false;
    bool insideFunction = false;
    bool insideLoop = false;
    std::optional<Type> returnType;
};

class SemanticAnalyzer : public ASTVisitor {
private:
    SymbolTable symbolTable;
    std::vector<SemanticContext> contextStack;
public:
    // SemanticAnalyzer() = default;
    SemanticAnalyzer(SymbolTable symbolTable) : symbolTable(std::move(symbolTable)) {
        ErrorReporter::resetCounts();
        contextStack.emplace_back(SemanticContext());
    }
    ~SemanticAnalyzer() override = default;

    void pushContext(SemanticContext ctx) {
        contextStack.push_back(std::move(ctx));
    }

    void popContext() {
        contextStack.pop_back();
    }

    SemanticContext& currentContext() {
        return contextStack.back();
    }

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