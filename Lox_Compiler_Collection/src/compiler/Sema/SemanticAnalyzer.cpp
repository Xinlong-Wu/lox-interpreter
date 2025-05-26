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
            symbol->markAsDefined();

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
        expr.setSymbol(symbol);
    }

    DEFINE_VISIT(BlockStmt) {
        // Enter a new scope for the block
        symbolTable.enterScope();

        // Visit each statement in the block
        for (auto& stmt : expr.getStatements()) {
            stmt->accept(*this);
        }

        // Exit the scope after visiting all statements
        symbolTable.exitScope();
    }

    DEFINE_VISIT(ClassDeclStmt) {
        // find superclass type if it exists
        std::shared_ptr<ClassType> superClassType = nullptr;
        if (expr.hasSuperclass()) {
            std::shared_ptr<Symbol> superClass = symbolTable.lookupSymbol(expr.getSuperclassName());
            if (superClass == nullptr) {
                ErrorReporter::reportError(&expr, "Superclass '" + expr.getSuperclassName() + "' is not defined");
                return;
            }
        }

        // create a new class type
        std::shared_ptr<ClassType> classType = std::make_shared<ClassType>(expr.getName(), superClassType);
        // create a new class symbol
        std::shared_ptr<Symbol> classSymbol = std::make_shared<Symbol>(expr.getName(), classType);
        // declare the class symbol in the symbol table
        if (!symbolTable.declare(classSymbol)) {
            ErrorReporter::reportError(&expr, "Class '" + expr.getName() + "' is already defined");
            return;
        }
        // mark the class symbol as defined
        classSymbol->markAsDefined();

        symbolTable.enterClassScope(expr.getName());
        // visit each field in the class
        for (const auto& field : expr.getFields()) {
            field.second->accept(*this);
        }
        expr.setClassScope(symbolTable.getCurrentScope());
        // exit the class scope
        symbolTable.exitScope();
    }

    DEFINE_VISIT(FunctionDecl) {
        // create a new function type
        std::vector<std::shared_ptr<Type>> parameterTypes;

        // add parameters to the function type
        for (const auto& param : expr.getParameters()) {
            assert (param->getType() != nullptr && "Parameter type should not be null");
            parameterTypes.push_back(param->getType());
        }

        // create a new function symbol
        std::shared_ptr<Symbol> funcSymbol = std::make_shared<Symbol>(expr.getName(),
            std::make_shared<FunctionType>(expr.getName(), std::move(parameterTypes)));
        // declare the function symbol in the symbol table
        if (!symbolTable.declare(funcSymbol)) {
            ErrorReporter::reportError(&expr, "Function '" + expr.getName() + "' is already defined");
            return;
        }
        // mark the function symbol as defined
        funcSymbol->markAsDefined();

        // enter a new function scope
        symbolTable.enterFunctionScope(expr.getName());
        // visit the function body
        expr.getBody()->accept(*this);
        // exit the function scope
        symbolTable.exitScope();
    }

    DEFINE_VISIT(IfStmt) {
        // resolve the condition expression
        if (expr.getCondition() == nullptr) {
            ErrorReporter::reportError(&expr, "'if' statement must have a condition");
            return;
        }
        expr.getCondition()->accept(*this);
        // check if the type of the condition is compatible with boolean
        std::shared_ptr<Type> conditionType = expr.getCondition()->getType();
        assert(conditionType != nullptr && "Condition type should not be null");
        if (!conditionType->isCompatible(std::make_shared<BoolType>())) {
            ErrorReporter::reportError(&expr, "Condition of 'if' statement must be a boolean expression");
            return;
        }

        // resolve the then and else branches
        expr.getThenBranch()->accept(*this);
        if (expr.hasElseBranch()) {
            expr.getElseBranch()->accept(*this);
        }

        // set the type of the if statement as nil
        // expr.setType(std::make_shared<NilType>());
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
        assert_not_reached("Unimplemented ReturnStmt visit");
    }


    // Expression visitors
    DEFINE_VISIT(ThisExpr) {
        std::shared_ptr<Scope> currentScope = symbolTable.getCurrentScope();
        if (!currentScope->isInClassScope() || !currentScope->isInFunctionScope()) {
            ErrorReporter::reportError(&expr, "'this' can only be used inside a class function");
            return;
        }
        assert(currentScope->getCurrentClassSymbol() != nullptr && "Current class symbol should not be null");
        expr.setType(currentScope->getCurrentClassSymbol()->getType());
    }

    DEFINE_VISIT(SuperExpr) {
        assert_not_reached("Unimplemented SuperExpr visit");
    }

    DEFINE_VISIT(GroupingExpr) {
        expr.getExpression()->accept(*this);
        // [Type inference] set the type of the grouping expression as the type of the inner expression
        std::shared_ptr<Type> innerType = expr.getExpression()->getType();
        if (innerType == nullptr) {
            // [Type inference] if the inner expression type is unknown, infer it as an unresolved type
            innerType = std::make_shared<UnresolvedType>("unknown");
            expr.getExpression()->setType(innerType);
        }
        else {
            // [Type inference] otherwise, set the grouping expression type as the inner expression type
            expr.setType(innerType);
        }
    }

    DEFINE_VISIT(CallExpr) {
        VariableExpr* callee = expr.getCallee();
        assert(callee != nullptr && "Callee should not be null");

        // resolve the callee
        callee->accept(*this);

        std::shared_ptr<FunctionType> calleeType = nullptr;
        // [Type inference] set the type of the call expression as the return type of the callee
        if (isa<FunctionType>(callee->getType())) {
            // if the callee is a function, set the call expression type as the return type of the function
            calleeType = dyn_cast<FunctionType>(callee->getType());
        }
        else if (isa<ClassType>(callee->getType())) {
            // if the callee is a class, set the call expression type as the class type
            std::shared_ptr<ClassType> classType = dyn_cast<ClassType>(callee->getType());
            calleeType = classType->getConstructor();
        }
        else {
            ErrorReporter::reportError(&expr, "Callee type should be a function type or class type");
            return;
        }
        expr.setType(calleeType);

        // resolve the arguments
        for (size_t i = 0; i < expr.getArguments().size(); ++i) {
            auto& arg = expr.getArguments()[i];
            arg->accept(*this);

            // check if the argument type is compatible with the callee type
            std::shared_ptr<Type>& paramType = calleeType->getParameter(i);
            assert (paramType != nullptr && !isa<UnresolvedType>(paramType.get()) && "Argument type should not be unresolved");
            std::shared_ptr<Type> argType = arg->getType();
            if (argType == nullptr) {
                // [Type inference] if the argument type is unknown, infer it as the parameter type
                arg->setType(paramType);
            }
            else if (!paramType->isCompatible(argType)) {
                ErrorReporter::reportError(arg.get(), "Incompatible types in function call");
                return;
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
        expr.setSymbol(symbol.get());

        if (!symbol->isDefinedSymbol()) {
            // ErrorReporter::reportError(&expr, "Variable '" + expr.getName() + "' is used before it is defined");
            return;
        }
        if (!symbol->isUsedSymbol()) {
            // mark the symbol as used
            symbol->markAsUsed();
        }
    }

    DEFINE_VISIT(LiteralExpr) {
        const std::string& value = expr.getValue();
        if (value.empty()) {
            // [Type inference] if the literal is empty, infer it as an unresolved type
            expr.setType(std::make_shared<UnresolvedType>("unknown"));
        }
        else if (value == "true" || value == "false") {
            // [Type inference] if the literal is a boolean, set the type as BoolType
            expr.setType(std::make_shared<BoolType>());
        }
        else if (value == "nil") {
            // [Type inference] if the literal is nil, set the type as NilType
            expr.setType(std::make_shared<NilType>());
        }
        else if (std::all_of(value.begin(), value.end(), ::isdigit)) {
            // [Type inference] if the literal is a number, set the type as NumberType
            expr.setType(std::make_shared<NumberType>());
        }
        else if (value.front() == '"' && value.back() == '"') {
            // [Type inference] if the literal is a string, set the type as StringType
            expr.setType(std::make_shared<StringType>());
        }
        else {
            ErrorReporter::reportError(&expr, "Unknown literal type: '" + value + "'");
        }
    }

    DEFINE_VISIT(NumberExpr) {
        assert_not_reached("Unimplemented NumberExpr visit");
    }

    DEFINE_VISIT(StringExpr) {
        expr.setType(std::make_shared<StringType>());
    }

    DEFINE_VISIT(UnaryExpr) {
        // resolve the right expression
        expr.getRight()->accept(*this);

        // check if the right type is compatible with the unary operator
        std::shared_ptr<Type> rightType = expr.getRight()->getType();
        if (rightType == nullptr) {
            // [Type inference] if the right type is unknown, infer it as an unresolved type
            rightType = std::make_shared<UnresolvedType>("unknown");
            expr.getRight()->setType(rightType);
        }
        else if (!isa<NumberType, BoolType, NilType, StringType>(rightType.get())) {
            // [Type inference] if the right type is not a number, boolean or nil, report an error
            ErrorReporter::reportError(&expr, "Invalid type for unary operator '" + convertTokenTypeToString(expr.getOpKind()) + "'");
            return;
        }

        // set the result type of the unary expression as the right type
        expr.setType(std::make_shared<BoolType>());
    }

    DEFINE_VISIT(BinaryExpr) {
        // resolve the left and right expressions
        expr.getLeft()->accept(*this);
        expr.getRight()->accept(*this);

        // check if the left and right types are compatible
        std::shared_ptr<Type> leftType = expr.getLeft()->getType();
        std::shared_ptr<Type> rightType = expr.getRight()->getType();
        if (leftType != nullptr && rightType != nullptr) {
            if (!leftType->isCompatible(rightType)) {
                ErrorReporter::reportError(&expr, "Incompatible types in binary expression");
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
            ErrorReporter::reportError(&expr, "Cannot perform operation on expressions of unknown type");
        }

        // set the result type of the binary expression as the left type
        expr.setType(leftType);
    }

    DEFINE_VISIT(AccessExpr) {
        assert_not_reached("Unimplemented AccessExpr visit");
    }

    DEFINE_VISIT(AssignExpr) {
        // resove the left and right expressions
        expr.getLeft()->accept(*this);
        expr.getRight()->accept(*this);

        // check if theris a type are compatible
        std::shared_ptr<Type> leftType = expr.getLeft()->getType();
        std::shared_ptr<Type> rightType = expr.getRight()->getType();
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
            return;
        }

        // set the result type of the assignment as the left type
        expr.setType(leftType);

        if (VariableExpr* var = dyn_cast<VariableExpr>(expr.getLeft())) {
            if (var->getSymbol() == nullptr) {
                ErrorReporter::reportError(&expr, "Variable '" + var->getName() + "' is not declared");
                return;
            }
            var->getSymbol()->markAsDefined();
        }
    }

    #undef DEFINE_VISIT

} // namespace lox

