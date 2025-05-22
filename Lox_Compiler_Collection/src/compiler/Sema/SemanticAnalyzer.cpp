#include "Common.h"
#include "Compiler/ErrorReporter.h"
#include "Compiler/Sema/SymbolTable.h"
#include "Compiler/Sema/SemanticAnalyzer.h"

#include <utility>

namespace lox
{
    #define DEFINE_VISIT(name) \
        void Sema::visit(name& expr)

    DEFINE_VISIT(ExpressionStmt) {
        assert_not_reached("Unimplemented ExpressionStmt visit");
    }

    DEFINE_VISIT(DeclarationStmt) {
        // Handle the declaration statement
        // For example, check if the variable is already declared
        std::cout << "Visiting DeclarationStmt" << std::endl;
    }

    DEFINE_VISIT(VarDeclStmt) {

        assert_not_reached("Unimplemented VarDeclStmt visit");
    }

    DEFINE_VISIT(BlockStmt) {
        assert_not_reached("Unimplemented BlockStmt visit");
    }

    DEFINE_VISIT(ClassDeclStmt) {
        assert_not_reached("Unimplemented ClassDeclStmt visit");
    }

    DEFINE_VISIT(FunctionDecl) {
        assert_not_reached("Unimplemented FunctionDecl visit");
    }

    DEFINE_VISIT(IfStmt) {
        assert_not_reached("Unimplemented IfStmt visit");
    }

    DEFINE_VISIT(WhileStmt) {
        // expr.getCondition()->accept(*this);
        // expr.getBody()->accept(*this);

        assert_not_reached("Unimplemented WhileStmt visit");
    }

    DEFINE_VISIT(ForStmt) {
        assert_not_reached("Unimplemented ForStmt visit");
    }

    DEFINE_VISIT(ReturnStmt) {
        // if (expr.getValue()) {
        //     expr.getValue()->accept(*this);
        // }
        std::cout << "Visiting ReturnStmt" << std::endl;
    }


    // Expression visitors
    DEFINE_VISIT(ThisExpr) {
        assert_not_reached("Unimplemented ThisExpr visit");
    }

    DEFINE_VISIT(SuperExpr) {
        assert_not_reached("Unimplemented SuperExpr visit");
    }

    DEFINE_VISIT(GroupingExpr) {
        assert_not_reached("Unimplemented GroupingExpr visit");
    }

    DEFINE_VISIT(CallExpr) {
        assert_not_reached("Unimplemented CallExpr visit");
    }

    DEFINE_VISIT(VariableExpr) {
        assert_not_reached("Unimplemented VariableExpr visit");
    }

    DEFINE_VISIT(LiteralExpr) {
        assert_not_reached("Unimplemented LiteralExpr visit");
    }

    DEFINE_VISIT(NumberExpr) {
        assert_not_reached("Unimplemented NumberExpr visit");
    }

    DEFINE_VISIT(StringExpr) {
        assert_not_reached("Unimplemented StringExpr visit");
    }

    DEFINE_VISIT(UnaryExpr) {
        assert_not_reached("Unimplemented UnaryExpr visit");
    }

    DEFINE_VISIT(BinaryExpr) {
        assert_not_reached("Unimplemented BinaryExpr visit");
    }

    DEFINE_VISIT(AccessExpr) {
        assert_not_reached("Unimplemented AccessExpr visit");
    }

    DEFINE_VISIT(AssignExpr) {
        assert_not_reached("Unimplemented AssignExpr visit");
    }

    #undef DEFINE_VISIT

} // namespace lox

