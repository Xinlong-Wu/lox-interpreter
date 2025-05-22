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
        expr.getExpression()->accept(*this);
    }

    DEFINE_VISIT(DeclarationStmt) {
        // Handle the declaration statement
        // For example, check if the variable is already declared
        std::cout << "Visiting DeclarationStmt" << std::endl;
    }

    DEFINE_VISIT(VarDeclStmt) {
        // create a new symbol for the variable
        std::shared_ptr<Symbol> symbol = expr.getSymbol();

        if (ExprBase* initializer = expr.getInitializer()) {
            // set the symbol as defined
            symbol->setDefined();

            // [Type inference] if the variable has an initializer, infer its type
            initializer->accept(*this);
            std::shared_ptr<Type> initializerType = initializer->getType();
            if (symbol->getType() != nullptr && !isa<UnresolvedType>(initializerType.get())) {
                if (!symbol->getType()->isCompatible(initializerType)) {
                    ErrorReporter::reportError(initializer, "Incompatible types in variable declaration");
                    return;
                }
            }
            else {
                symbol->setType(initializerType);
            }
        }
        symbolTable.declare(symbol);
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
        VariableExpr* callee = expr.getCallee();
        assert(callee != nullptr && "Callee should not be null");

        // resolve the callee
        callee->accept(*this);

        // [Type inference] set the type of the call expression as the return type of the callee
        std::shared_ptr<FunctionType> calleeType = dyn_cast<FunctionType>(callee->getType());
        if (calleeType == nullptr) {
            ErrorReporter::reportError(callee, "Undefined function '" + callee->getName() + "'");
            return;
        }
        expr.setType(calleeType->getReturnType());

        // resolve the arguments
        for (size_t i = 0; i < expr.getArguments().size(); ++i) {
            auto& arg = expr.getArguments()[i];
            arg->accept(*this);

            // check if the argument type is compatible with the callee type
            std::shared_ptr<Type> argType = arg->getType();
            if (argType != nullptr && calleeType->getParameter(i) != nullptr) {
                if (!calleeType->getParameter(i)->isCompatible(argType)) {
                    ErrorReporter::reportError(arg, "Incompatible types in function call");
                    return;
                }
            }
        }
        

    }

    DEFINE_VISIT(VariableExpr) {
        // check if the variable is declared
        std::shared_ptr<Symbol> symbol = symbolTable.lookupSymbol(expr.getName());
        if (symbol == nullptr) {
            ErrorReporter::reportError(&expr, "Use of Undeclared Variable '" + expr.getName() + "'");
            return;
        }

        // set the type of the variable as the symbol type
        expr.setType(symbol->getType());
    }

    DEFINE_VISIT(LiteralExpr) {
        assert_not_reached("Unimplemented LiteralExpr visit");
    }

    DEFINE_VISIT(NumberExpr) {
        assert_not_reached("Unimplemented NumberExpr visit");
    }

    DEFINE_VISIT(StringExpr) {
        expr.setType(std::make_shared<StringType>());
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
        // resove the left and right expressions
        expr.getLeft()->accept(*this);
        expr.getRight()->accept(*this);

        // check if theris a type are compatible
        std::shared_ptr<Type>& leftType = expr.getLeft()->getType();
        std::shared_ptr<Type>& rightType = expr.getRight()->getType();
        if (leftType != nullptr && rightType != nullptr) {
            if (!leftType->isCompatible(rightType)) {
                ErrorReporter::reportError(&expr, "Incompatible types in assignment");
                return;
            }
        }
        else if (leftType == nullptr && rightType != nullptr) {
            // [Type inference] if the left type is unknown, infer left as the right type
            expr.getLeft()->setType(rightType);
        }
        else if (leftType != nullptr && rightType == nullptr) {
            // [Type inference] if the right type is unknown, infer right as the left type
            expr.getRight()->setType(leftType);
        }
        else {
            ErrorReporter::reportError(&expr, "Cannot assign an expression of unknown type");
        }

        // set the result type of the assignment as the left type
        expr.setType(leftType);
    }

    #undef DEFINE_VISIT

} // namespace lox

