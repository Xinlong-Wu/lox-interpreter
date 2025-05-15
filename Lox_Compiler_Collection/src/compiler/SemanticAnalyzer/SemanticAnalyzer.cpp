#include "Common.h"
#include "Compiler/SemanticAnalyzer/SemanticAnalyzer.h"

#include <utility>

namespace lox
{
    #define DEFINE_VISIT(name) \
        void SemanticAnalyzer::visit(name& expr)

    DEFINE_VISIT(ExpressionStmt) {
        // expr.getExpression()->accept(*this);
        // expr.setType(expr.getExpression()->getType());

        std::cout << "Visiting ExpressionStmt" << std::endl;
    }

    DEFINE_VISIT(DeclarationStmt) {
        // Handle the declaration statement
        // For example, check if the variable is already declared
        std::cout << "Visiting DeclarationStmt" << std::endl;
    }

    DEFINE_VISIT(VarDeclStmt) {
        // Handle the variable declaration statement
        // For example, check if the variable is already declared
        std::cout << "Visiting VarDeclStmt" << std::endl;
    }

    DEFINE_VISIT(BlockStmt) {
        // for (const auto& stmt : expr.getStatements()) {
        //     stmt->accept(*this);
        // }
        std::cout << "Visiting BlockStmt" << std::endl;
    }

    DEFINE_VISIT(ClassDeclStmt) {
        // Handle the class declaration statement
        // For example, check if the class is already declared
        std::cout << "Visiting ClassDeclStmt" << std::endl;
    }

    DEFINE_VISIT(FunctionDecl) {
        // Handle the function declaration
        // For example, check if the function is already declared
        std::cout << "Visiting FunctionDecl" << std::endl;
    }

    DEFINE_VISIT(IfStmt) {
        // expr.getCondition()->accept(*this);
        // expr.getThenBranch()->accept(*this);
        // if (expr.getElseBranch()) {
        //     expr.getElseBranch()->accept(*this);
        // }
        std::cout << "Visiting IfStmt" << std::endl;
    }

    DEFINE_VISIT(WhileStmt) {
        // expr.getCondition()->accept(*this);
        // expr.getBody()->accept(*this);

        std::cout << "Visiting WhileStmt" << std::endl;
    }

    DEFINE_VISIT(ForStmt) {
        // if (expr.getInitializer()) {
        //     expr.getInitializer()->accept(*this);
        // }
        // if (expr.getCondition()) {
        //     expr.getCondition()->accept(*this);
        // }
        // if (expr.getIncrement()) {
        //     expr.getIncrement()->accept(*this);
        // }
        // expr.getBody()->accept(*this);
        std::cout << "Visiting ForStmt" << std::endl;
    }

    DEFINE_VISIT(ReturnStmt) {
        // if (expr.getValue()) {
        //     expr.getValue()->accept(*this);
        // }
        std::cout << "Visiting ReturnStmt" << std::endl;
    }


    // Expression visitors
    DEFINE_VISIT(ThisExpr) {
        // Handle the 'this' expression
        // For example, check if 'this' is used in a valid context
        std::cout << "Visiting ThisExpr" << std::endl;
    }

    DEFINE_VISIT(SuperExpr) {
        // Handle the 'super' expression
        // For example, check if 'super' is used in a valid context
        std::cout << "Visiting SuperExpr" << std::endl;
    }

    DEFINE_VISIT(GroupingExpr) {
        expr.getExpression()->accept(*this);
        expr.setType(expr.getExpression()->getType());
    }

    DEFINE_VISIT(CallExpr) {
        expr.getCallee()->accept(*this);
        for (const auto& arg : expr.getArguments()) {
            arg->accept(*this);
        }
        assert_not_reached("return type of call expression is not set");
        // expr.setType(expr.getCallee()->getType());
    }

    DEFINE_VISIT(VariableExpr) {
        // Check if the variable is defined in the symbol table
        auto it = symbolTable.find(expr.getSymName());
        if (it != symbolTable.end()) {
            expr.setType(it->second);
            return;
        }
        assert_not_reached("Variable not found in symbol table");
    }

    DEFINE_VISIT(LiteralExpr) {
        if (expr.getValue() == "false" || expr.getValue() == "true") {
            expr.setType(lox::Type::TYPE_BOOL);
        } else if (expr.getValue() == "nil") {
            expr.setType(lox::Type::TYPE_NONE);
        }
        assert_not_reached("unexpected literal type");
    }

    DEFINE_VISIT(NumberExpr) {
        expr.setType(lox::Type::TYPE_NUMBER);
    }

    DEFINE_VISIT(StringExpr) {
        expr.setType(lox::Type::TYPE_STRING);
    }

    DEFINE_VISIT(UnaryExpr) {
        expr.getRight()->accept(*this);
        switch (expr.getKind())
        {
        case lox::TokenType::TOKEN_BANG:
            expr.setType(lox::Type::TYPE_BOOL);
            break;
        case lox::TokenType::TOKEN_MINUS:
            expr.setType(lox::Type::TYPE_NUMBER);
            break;
        default:
            assert_not_reached("unexpected unary operator");
            break;
        }
    }

    DEFINE_VISIT(BinaryExpr) {
        // Handle the binary expression
        // For example, check if the binary operation is valid
        std::cout << "Visiting BinaryExpr" << std::endl;
    }

    DEFINE_VISIT(AccessExpr) {
        // Handle the access expression
        // For example, check if the property being accessed is valid
        std::cout << "Visiting AccessExpr" << std::endl;
    }

    DEFINE_VISIT(AssignExpr) {
        // Handle the assignment expression
        // For example, check if the assignment is valid
        std::cout << "Visiting AssignExpr" << std::endl;
    }

    #undef DEFINE_VISIT

} // namespace lox

