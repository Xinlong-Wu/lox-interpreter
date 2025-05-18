#include "Common.h"
#include "Compiler/ErrorReporter.h"
#include "Compiler/SemanticAnalyzer/SymbolTable.h"
#include "Compiler/SemanticAnalyzer/SemanticAnalyzer.h"

#include <utility>

namespace lox
{
    #define DEFINE_VISIT(name) \
        void SemanticAnalyzer::visit(name& expr)

    DEFINE_VISIT(ExpressionStmt) {
        expr.getExpression()->accept(*this);
        if (expr.getExpression()->getType() == lox::Type::TYPE_UNKNOWN) {
            ErrorReporter::reportError(&expr, "Unable to determine type of expression");
            return;
        }
        expr.setType(expr.getExpression()->getType());
    }

    DEFINE_VISIT(DeclarationStmt) {
        // Handle the declaration statement
        // For example, check if the variable is already declared
        std::cout << "Visiting DeclarationStmt" << std::endl;
    }

    DEFINE_VISIT(VarDeclStmt) {
        if (expr.getInitializer()) {
            if (expr.getInitializer()->getType() == lox::Type::TYPE_UNKNOWN) {
                // Handle the case where the initializer is not yet evaluated
                expr.getInitializer()->accept(*this);
            }
            assert(expr.getType() == lox::Type::TYPE_UNKNOWN);
            expr.setType(expr.getInitializer()->getType());
        }

        // Declare the variable in the symbol table
        if (!symbolTable.declare(std::make_shared<Symbol>(expr.getName(), expr.getType()))) {
            ErrorReporter::reportError(&expr, "Variable already declared in this scope");
            return;
        }
    }

    DEFINE_VISIT(BlockStmt) {
        for (auto& stmt : expr.statements) {
            if (ReturnStmt* returnStmt = dyn_cast<ReturnStmt>(stmt.get())) {
                if (expr.getType() == lox::Type::TYPE_UNKNOWN) {
                    expr.setType(returnStmt->getType());
                }
                else if (expr.getType() != returnStmt->getType()) {
                    ErrorReporter::reportError(returnStmt, "Return conflict type: expected " +
                        convertTypeToString(expr.getType()) + ", but got " +
                        convertTypeToString(returnStmt->getType()));
                    return;
                }
            }
            stmt->accept(*this);
        }
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
        VariableExpr* callee = cast<VariableExpr>(expr.getCallee());
        FunctionSymbol* func = cast<FunctionSymbol>(callee->getSymbol());
        if (func->getParameterCount() != expr.getArguments().size()) {
            ErrorReporter::reportError(&expr, "Function '" + callee->getSymName() + "' expects " +
                std::to_string(func->getParameterCount()) + " arguments, but got " +
                std::to_string(expr.getArguments().size()));
            return;
        }

        for (size_t i = 0; i < func->getParameterCount(); ++i) {
            ExprBase *arg = expr.getArgument(i);
            lox::Type paramType = func->getParameterType(i);
            arg->accept(*this);
            if (arg->getType() != paramType) {
                ErrorReporter::reportError(&expr, "Argument " + std::to_string(i) + " of function '" +
                    callee->getSymName() + "' is of type " +
                    convertTypeToString(arg->getType()) + ", but expected " +
                    convertTypeToString(paramType));
                return;
            }
        }
        expr.setType(func->type);
    }

    DEFINE_VISIT(VariableExpr) {
        // Check if the variable is defined in the symbol table
        Symbol* sym = symbolTable.lookup(expr.getSymName());
        if (!sym) {
            ErrorReporter::reportError(&expr, "Undefined variable '" + expr.getSymName() + "'");
            return;
        }
        // Set the type of the variable expression based on the symbol table
        expr.setSymbol(sym);
    }

    DEFINE_VISIT(LiteralExpr) {
        // if (expr.getValue() == "false" || expr.getValue() == "true") {
        //     expr.setType(lox::Type::TYPE_BOOL);
        // } else if (expr.getValue() == "nil") {
        //     expr.setType(lox::Type::TYPE_UNKNOWN);
        // }
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
        switch (expr.getOpKind())
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
        ExprBase* left = expr.getLeft();
        ExprBase* right = expr.getRight();
        left->accept(*this);
        right->accept(*this);
        
        if (left->getType() == lox::Type::TYPE_UNKNOWN) {
            ErrorReporter::reportError(&expr, "Unable to determine type of expression");
            return;
        }
        if (right->getType() == lox::Type::TYPE_UNKNOWN) {
            ErrorReporter::reportError(&expr, "unknown type for right side of binary expression");
            return;
        }

        switch (expr.getOpKind())
        {
        case lox::TokenType::TOKEN_PLUS:
            if (left->getType() == lox::Type::TYPE_STRING && right->getType() == lox::Type::TYPE_STRING) {
                expr.setType(lox::Type::TYPE_STRING);
            } else if (left->getType() == lox::Type::TYPE_NUMBER && right->getType() == lox::Type::TYPE_NUMBER) {
                expr.setType(lox::Type::TYPE_NUMBER);
            } else {
                ErrorReporter::reportError(&expr, "Unable to add " +
                    convertTypeToString(left->getType()) + " and " +
                    convertTypeToString(right->getType()));
            }
            return;
        case lox::TokenType::TOKEN_MINUS:
        case lox::TokenType::TOKEN_SLASH:
        case lox::TokenType::TOKEN_STAR:
            if (left->getType() == lox::Type::TYPE_NUMBER && right->getType() == lox::Type::TYPE_NUMBER) {
                expr.setType(lox::Type::TYPE_NUMBER);
            } else {
                ErrorReporter::reportError(&expr, "Unable to perform " +
                    convertTokenTypeToString(expr.getOpKind()) + " on " +
                    convertTypeToString(left->getType()) + " and " +
                    convertTypeToString(right->getType()));
            }
            return;
        
        default:
            ErrorReporter::reportError(&expr, "Unexpected binary operator: " +
                convertTokenTypeToString(expr.getOpKind()));
            return;
        }
    }

    DEFINE_VISIT(AccessExpr) {
        // Handle the access expression
        // For example, check if the property being accessed is valid
        std::cout << "Visiting AccessExpr" << std::endl;
    }

    DEFINE_VISIT(AssignExpr) {
        expr.getLeft()->accept(*this);
        expr.getRight()->accept(*this);
        if (expr.getLeft()->getType() == lox::Type::TYPE_UNKNOWN) {
            ErrorReporter::reportError(&expr, "unknown type for left side of assignment");
            return;
        }
        if (expr.getRight()->getType() == lox::Type::TYPE_UNKNOWN) {
            ErrorReporter::reportError(&expr, "unknown type for right side of assignment");
            return;
        }

        if (expr.getLeft()->getType() != expr.getRight()->getType()) {
            ErrorReporter::reportError(&expr, "unable to assign a " + convertTypeToString(expr.getRight()->getType()) +
                " to a " + convertTypeToString(expr.getLeft()->getType()));
            return;
        }

        expr.setType(expr.getLeft()->getType());
    }

    #undef DEFINE_VISIT

} // namespace lox

