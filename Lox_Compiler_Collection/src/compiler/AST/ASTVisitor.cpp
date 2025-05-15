
#include "Compiler/AST/Expr.h"
#include "Compiler/AST/ASTVisitor.h"

#define ACCEPT(name) \
        void name::accept(ASTVisitor& visitor) { visitor.visit(*this); }

namespace lox {

    ACCEPT(ThisExpr)
    ACCEPT(SuperExpr)
    ACCEPT(GroupingExpr)
    ACCEPT(CallExpr)
    ACCEPT(VariableExpr)
    ACCEPT(LiteralExpr)
    ACCEPT(NumberExpr)
    ACCEPT(StringExpr)
    ACCEPT(UnaryExpr)
    ACCEPT(BinaryExpr)
    ACCEPT(AccessExpr)
    ACCEPT(AssignExpr)

    ACCEPT(ExpressionStmt)
    ACCEPT(DeclarationStmt)
    ACCEPT(VarDeclStmt)
    ACCEPT(BlockStmt)
    ACCEPT(ClassDeclStmt)
    ACCEPT(FunctionDecl)
    ACCEPT(IfStmt)
    ACCEPT(WhileStmt)
    ACCEPT(ForStmt)
    ACCEPT(ReturnStmt)

}

#undef ACCEPT
