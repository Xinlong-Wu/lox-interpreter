
#include "Compiler/Sema/SymbolResolver.h"

#include <memory>

using namespace std;
using namespace lox;

DEFINE_VISIT(SymbolResolver, ThisExpr) {
    shared_ptr<Symbol> currentClassSymbol = symbolTable.getCurrentScope()->getCurrentClassSymbol();
    if (currentClassSymbol == nullptr && symbolTable.getCurrentScope()->inFunctionScope()) {
        ErrorReporter::reportError(&expr, "'this' can only be used inside a class method");
        return;
    }

    // Set the type of the 'this' expression as the current class type
    ClassType *classType = dyn_cast<ClassType>(currentClassSymbol->getType().get());
    if (classType == nullptr) {
        ErrorReporter::reportError(&expr, "Class '" + classType->getName() + "' does not have a type");
        return;
    }
    expr.setType(classType->getInstanceType());
}

DEFINE_VISIT(SymbolResolver, SuperExpr) {
    shared_ptr<Symbol> currentClassSymbol = symbolTable.getCurrentScope()->getCurrentClassSymbol();
    if (currentClassSymbol == nullptr && symbolTable.getCurrentScope()->inFunctionScope()) {
        ErrorReporter::reportError(&expr, "'super' can only be used inside a class method");
        return;
    }

    // Check if the current class has a superclass
    ClassType *classType = dyn_cast<ClassType>(currentClassSymbol->getType().get());
    if (classType == nullptr) {
        ErrorReporter::reportError(&expr, "Class '" + classType->getName() + "' does not have a superclass");
        return;
    }

    // Set the type of the super expression as the superclass type
    expr.setType(classType->getSuperClass()->getInstanceType());
}

DEFINE_VISIT(SymbolResolver, GroupingExpr) {
    // Visit the inner expression
    expr.getExpression()->accept(*this);
    
    // Set the type of the grouping expression as the type of the inner expression
    expr.setType(expr.getExpression()->getType());
}

DEFINE_VISIT(SymbolResolver, CallExpr) {
    // Visit the callee expression
    expr.getCallee()->accept(*this);
    
    // Check if the callee is a variable or access expression
    ExprBase *callee = expr.getCallee();
    shared_ptr<Type> calleeType = callee->getType();
    
    if (calleeType == nullptr) {
        assert_not_reached("Callee type should not be null");
    }

    // Visit each argument expression
    for (auto &arg : expr.getArguments()) {
        arg->accept(*this);
    }
    
    if (auto functionType = dyn_cast<FunctionType>(calleeType)) {
        // If the callee is a function type, set the call expression type as the return type of the function
        expr.setType(functionType->getReturnType());
    } else if (auto classType = dyn_cast<ClassType>(calleeType)) {
        // If the callee is a class type, check if it has a constructor
        assert (classType->hasConstructor()&& "Class type should have a constructor");
        expr.setType(classType->getInstanceType());
    }
    else {
        expr.setType(UnresolvedType::getInstance()); // Default to UnresolvedType
    }
}

DEFINE_VISIT(SymbolResolver, VariableExpr) {
    // Check if the variable is declared
    shared_ptr<Symbol> symbol = symbolTable.lookupSymbol(expr.getName());
    if (symbol == nullptr) {
        ErrorReporter::reportError(&expr, "Use of Undeclared Variable '" + expr.getName() + "'");
        return;
    }

    // Set the type of the variable as the symbol type
    expr.setSymbol(symbol.get());

    if (!symbol->isDefinedSymbol()) {
        // If the variable is used before it is defined, report an error
        ErrorReporter::reportError(&expr, "Variable '" + expr.getName() + "' is used before it is defined");
        return;
    }
    
    // Mark the symbol as used
    symbol->markAsUsed();
}

DEFINE_VISIT(SymbolResolver, LiteralExpr) {
    // Check the value of the literal expression
    const string &value = expr.getValue();
    
    if (value == "true" || value == "false") {
        // If the literal is a boolean, set the type as BoolType
        expr.setType(BoolType::getInstance());
    } else if (value == "nil") {
        // If the literal is nil, set the type as NilType
        expr.setType(NilType::getInstance());
    } else if (all_of(value.begin(), value.end(), ::isdigit)) {
        // If the literal is a number, set the type as NumberType
        expr.setType(NumberType::getInstance());
    } else if (value.front() == '"' && value.back() == '"') {
        // If the literal is a string, set the type as StringType
        expr.setType(StringType::getInstance());
    } else {
        ErrorReporter::reportError(&expr, "Unknown literal type: '" + value + "'");
    }
}

DEFINE_VISIT(SymbolResolver, NumberExpr) {
    // Set the type of the number expression as NumberType
    expr.setType(NumberType::getInstance());
}

DEFINE_VISIT(SymbolResolver, StringExpr) {
    // Set the type of the string expression as StringType
    expr.setType(StringType::getInstance());
}

DEFINE_VISIT(SymbolResolver, UnaryExpr) {
    // Visit the operand expression
    expr.getRight()->accept(*this);
    // Set the type of the unary expression with the type of the operand

    switch (expr.getOpKind()) {
        case TokenType::TOKEN_MINUS:
            // If the operand is a number, set the type as NumberType
            expr.setType(expr.getRight()->getType());
            break;
        case TokenType::TOKEN_BANG:
            // If the operand is a boolean, set the type as BoolType
            expr.setType(BoolType::getInstance());
            break;
        default:
            assert_not_reached("Unimplemented UnaryExpr visit");
    }
}

DEFINE_VISIT(SymbolResolver, BinaryExpr) {
    // Visit the left and right expressions
    expr.getLeft()->accept(*this);
    expr.getRight()->accept(*this);

    // Get the types of the left and right expressions
    shared_ptr<Type> leftType = expr.getLeft()->getType();
    shared_ptr<Type> rightType = expr.getRight()->getType();

    // If both types are known, check compatibility
    if (leftType == rightType) {
        if (leftType->isCompatible(rightType)) {
            expr.setType(leftType); // Set the result type as the left type
            return;
        }
    }

    // If either type is unknown, set the result type as UnresolvedType
    expr.setType(UnresolvedType::getInstance());
}

DEFINE_VISIT(SymbolResolver, AccessExpr) {
    // Visit the base expression
    expr.getBase()->accept(*this);

    // Get the type of the base expression
    shared_ptr<Type> baseType = expr.getBase()->getType();
    if (baseType == nullptr) {
        ErrorReporter::reportError(&expr, "Base expression type is unknown");
        return;
    }

    // Check if the base type is a class type
    if (auto classType = dyn_cast<ClassType>(baseType)) {
        // Get the property type from the class type
        shared_ptr<Symbol> propertySymbol = classType->getProperty(expr.getProperty());
        if (propertySymbol == nullptr) {
            ErrorReporter::reportError(&expr, "Property '" + expr.getProperty() + "' is not defined in class '" + classType->getName() + "'");
            return;
        }
        
        shared_ptr<Type> propertyType = classType->getProperty(expr.getProperty())->getType();
        if (propertyType == nullptr) {
            ErrorReporter::reportError(&expr, "Property '" + expr.getProperty() + "' is not defined in class '" + classType->getName() + "'");
            return;
        }
        expr.setType(propertyType);
    }
    else if (isa<UnresolvedType>(baseType)) {
        // If the base type is unresolved, we cannot determine the property type
        expr.setType(UnresolvedType::getInstance());
    }
    else {
        // If the base type is not a class or function type, report an error
        ErrorReporter::reportError(&expr, "Base expression must be a class type");
    }
}

DEFINE_VISIT(SymbolResolver, AssignExpr) {
    // Visit the left and right expressions
    expr.getRight()->accept(*this);
    // expr.getLeft()->accept(*this);

    ExprBase *leftExpr = expr.getLeft();
    shared_ptr<Type> leftType = nullptr;
    if (isa<VariableExpr>(leftExpr)) {
        // If the left expression is a variable, resolve it
        VariableExpr *varExpr = cast<VariableExpr>(leftExpr);
        shared_ptr<Symbol> symbol = symbolTable.lookupSymbol(varExpr->getName());
        if (symbol == nullptr) {
            ErrorReporter::reportError(&expr, "Use of Undeclared Variable '" + varExpr->getName() + "'");
            return;
        }
        symbol->markAsDefined();
        varExpr->setSymbol(symbol.get());
        leftType = symbol->getType();
    } else if (isa<AccessExpr>(leftExpr)) {
        // If the left expression is an access expression, resolve it
        leftExpr->accept(*this);
        leftType = leftExpr->getType();
    } else if (isa<GroupingExpr>(leftExpr)) {
        // If the left expression is a grouping expression, resolve it
        GroupingExpr *groupExpr = cast<GroupingExpr>(leftExpr);
        groupExpr->getExpression()->accept(*this);
        leftType = groupExpr->getExpression()->getType();
    } else if (isa<ThisExpr, SuperExpr>(leftExpr)) {
        ErrorReporter::reportError(&expr, "Cannot assign to 'this' or 'super'");
        return;
    } else {
        assert_not_reached("Unsupported left expression type in assignment");
    }

    // Set the result type of the assignment expression as the left type
    expr.setType(leftType);
}

DEFINE_VISIT(SymbolResolver, ExpressionStmt) {
    // Visit the expression in the statement
    expr.getExpression()->accept(*this);
}

DEFINE_VISIT(SymbolResolver, DeclarationStmt) {
    assert_not_reached("Unimplemented DeclarationStmt visit");
}

DEFINE_VISIT(SymbolResolver, VarDeclStmt) {
    shared_ptr<Type> varType = nullptr;

    // Create a new symbol for the variable
    shared_ptr<Symbol> symbol = make_shared<Symbol>(expr.getName(), varType);
    
    // Declare the variable in the symbol table
    if (!symbolTable.declare(symbol)) {
        ErrorReporter::reportError(&expr, "Variable '" + expr.getName() + "' is already defined");
        return;
    }

    if (ExprBase *initializer = expr.getInitializer()) {
        // Set the symbol as defined
        symbol->markAsDefined();

        // [Type inference] if the variable has an initializer, infer its type
        initializer->accept(*this);
        shared_ptr<Type> initializerType = initializer->getType();
        varType = initializerType;
    }

    if (varType == nullptr) {
        // If the variable type is not set, use UnresolvedType as default
        varType = UnresolvedType::getInstance();
    }
    symbol->setType(varType);
    expr.setSymbol(symbol);
}

DEFINE_VISIT(SymbolResolver, BlockStmt) {
    // Enter a new scope for the block
    symbolTable.enterScope("block");

    // Visit each statement in the block
    for (auto &stmt : expr.getStatements()) {
        stmt->accept(*this);
    }

    // Set the scope of the block
    expr.setScope(symbolTable.getCurrentScope());

    // Exit the scope after visiting all statements
    symbolTable.exitScope();
}

DEFINE_VISIT(SymbolResolver, ClassDeclStmt) {
    shared_ptr<ClassType> superClassType = nullptr;
    if (expr.hasSuperclass()) {
        shared_ptr<Symbol> superClass = symbolTable.lookupSymbol(expr.getSuperclassName());
        if (superClass == nullptr) {
            ErrorReporter::reportError(&expr, "Superclass '" + expr.getSuperclassName() + "' is not defined");
            return;
        }
        if (isa<ClassType>(superClass->getType())) {
            superClassType = cast<ClassType>(superClass->getType());
        } else {
            ErrorReporter::reportError(&expr, "Superclass '" + expr.getSuperclassName() + "' is not a class");
            return;
        }
    }

    // Create a new class symbol
    shared_ptr<Symbol> classSymbol = make_shared<Symbol>(expr.getName());
    shared_ptr<ClassType> classType = make_shared<ClassType>(expr.getName(), superClassType);
    classSymbol->setType(classType);
    classSymbol->markAsDefined();
    
    // Declare the class in the symbol table
    if (!symbolTable.declare(classSymbol)) {
        ErrorReporter::reportError(&expr, "Class '" + expr.getName() + "' is already defined");
        return;
    }

    // Enter a new scope for the class
    symbolTable.enterClassScope(expr.getName());

    // Set the class properties
    classType->setProperties(symbolTable.getCurrentScope());

    // Declare the class fields and methods
    for (const auto &field : expr.getFields()) {
        field.second->accept(*this);
    }

    for (const auto &method : expr.getMethods()) {
        method.second->accept(*this);
    }

    if (!classType->hasConstructor()) {
        // If the class does not have a constructor, create a default constructor
        shared_ptr<ConstructorType> constructorType = make_shared<ConstructorType>(classSymbol->getName(),
                                                                                  classType->getInstanceType());
        shared_ptr<Symbol> constructorSymbol = make_shared<Symbol>(classSymbol->getName(), constructorType);
        symbolTable.declare(constructorSymbol);
    }

    // Exit the class scope
    symbolTable.exitScope();

    expr.setSymbol(classSymbol);
}

DEFINE_VISIT(SymbolResolver, FunctionDecl) {
    shared_ptr<FunctionType> funcType = nullptr;

    std::string functionName = expr.getName();
    bool isConstructor = false;
    shared_ptr<Symbol> currentClassSymbol = symbolTable.getCurrentScope()->getCurrentClassSymbol();
    if (currentClassSymbol) {
        // If we are in a class scope, the function name is the class name
        if (functionName == currentClassSymbol->getName()) {
            isConstructor = true;
        }
    }

    // Create or retrieve the function symbol
    shared_ptr<Symbol> symbol = symbolTable.lookupSymbol(functionName);

    if (isConstructor) {
        shared_ptr<ClassType> classType = cast<ClassType>(symbol->getType());
        // If the symbol is a class, we can treat it as a constructor
        symbol = classType->getConstructor();
    }

    if (symbol == nullptr) {
        // If the function is not defined, create a new symbol
        symbol = make_shared<Symbol>(functionName);
        // declare it in the symbol table
        symbolTable.declare(symbol);

        // Create a new function type for the symbol
        if (isConstructor) {
            // If it's a constructor, create a ConstructorType
            funcType = make_shared<ConstructorType>(functionName,
                                                    cast<ClassType>(currentClassSymbol->getType())->getInstanceType());
        } else {
            // Otherwise, create a regular FunctionType
            funcType = make_shared<FunctionType>(functionName);
        }
        symbol->setType(funcType);
    } else if (isa<FunctionType>(symbol->getType())) {
        // If the function is already defined, retrieve its type
        funcType = cast<FunctionType>(symbol->getType());
    }
    else if (isa<UnresolvedType>(symbol->getType())) {
        // If the symbol is unresolved, we can still declare it as a function
        funcType = make_shared<FunctionType>(functionName);
        symbol->setType(funcType);
    }
    else {
        // If the symbol is not a function, report an error
        ErrorReporter::reportError(&expr, "Symbol '" + functionName + "' is not a function");
        return;
    }

    // Set the symbol for the function declaration
    expr.setSymbol(symbol);
    symbol->markAsDefined();
    
    // Enter a new scope for the function
    symbolTable.enterFunctionScope(functionName);
    
    // Declare the function parameters in the current scope
    std::vector<shared_ptr<Type>> parameterTypes;
    for (const auto &param : expr.getParameters()) {
        // param->accept(*this);
        // Create a new symbol for the parameter
        shared_ptr<Symbol> paramSymbol = make_shared<Symbol>(param->getName(), UnresolvedType::getInstance());
        paramSymbol->markAsDefined();

        // Declare the parameter in the current scope
        if (!symbolTable.declare(paramSymbol)) {
            ErrorReporter::reportError(&expr, "Parameter '" + param->getName() + "' is already defined");
            return;
        }
        // Add the parameter to the function type
        parameterTypes.push_back(paramSymbol->getType());
    }
    
    // Visit the function body
    for (auto &stmt : expr.getBody()->getStatements()) {
        stmt->accept(*this);
    }
    
    // Set the function's scope
    expr.getBody()->setScope(symbolTable.getCurrentScope());

    shared_ptr<Type> returnType = UnresolvedType::getInstance();
    if (isConstructor) {
        // If it's a constructor, the return type is the class instance type
        returnType = cast<ClassType>(currentClassSymbol->getType())->getInstanceType();
    }
    FunctionType::Signature signature(parameterTypes, returnType);
    // Add the signature to the function type
    funcType->addOverload(signature);
    
    // Exit the function scope
    symbolTable.exitScope();
}

DEFINE_VISIT(SymbolResolver, IfStmt) {
    symbolTable.enterScope("if_statement");
    expr.getCondition()->accept(*this);
    
    expr.getThenBranch()->accept(*this);

    if (expr.hasElseBranch()) {
        expr.getElseBranch()->accept(*this);
    }

    expr.setScope(symbolTable.getCurrentScope());
    symbolTable.exitScope();
    
    // expr.setType(NilType::getInstance()); // For now, we assume the if statement returns nil
}

DEFINE_VISIT(SymbolResolver, WhileStmt) {
    symbolTable.enterScope("while_loop");
    expr.getCondition()->accept(*this);

    for (auto &stmt : expr.getBody()->getStatements()) {
        stmt->accept(*this);
    }
    expr.getBody()->setScope(symbolTable.getCurrentScope());
    symbolTable.exitScope();
    
    // expr.setType(NilType::getInstance()); // For now, we assume the while loop returns nil
}

DEFINE_VISIT(SymbolResolver, ForStmt) {
    symbolTable.enterScope("for_loop");
    if (expr.getInitializer()) {
        expr.getInitializer()->accept(*this);
    }
    if (expr.getCondition()) {
        expr.getCondition()->accept(*this);
    }
    if (expr.getIncrement()) {
        expr.getIncrement()->accept(*this);
    }

    for (auto &stmt : expr.getBody()->getStatements()) {
        stmt->accept(*this);
    }
    
    expr.getBody()->setScope(symbolTable.getCurrentScope());
    symbolTable.exitScope();

    // expr.setType(NilType::getInstance()); // For now, we assume the for loop returns nil
}

DEFINE_VISIT(SymbolResolver, ReturnStmt) {
    expr.getValue()->accept(*this);
}
