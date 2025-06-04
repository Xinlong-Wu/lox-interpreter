#include "Compiler/Sema/SemanticAnalyzer.h"
#include "Common.h"
#include "Compiler/ErrorReporter.h"
#include "Compiler/Sema/SymbolTable.h"

#include <utility>

namespace lox {
#define DEFINE_VISIT(name) void Sema::visit(name &expr)

DEFINE_VISIT(ExpressionStmt) { expr.getExpression()->accept(*this); }

DEFINE_VISIT(DeclarationStmt) {
  // Handle the declaration statement
  // For example, check if the variable is already declared
  std::cout << "Visiting DeclarationStmt" << std::endl;
}

DEFINE_VISIT(VarDeclStmt) {
  // create a new symbol for the variable
  std::shared_ptr<Symbol> symbol = expr.getSymbol();

  if (ExprBase *initializer = expr.getInitializer()) {
    // set the symbol as defined
    symbol->markAsDefined();

    // [Type inference] if the variable has an initializer, infer its type
    initializer->accept(*this);
    std::shared_ptr<Type> initializerType = initializer->getType();
    if (symbol->getType() != nullptr &&
        !isa<UnresolvedType>(initializerType.get())) {
      if (!symbol->getType()->isCompatible(initializerType)) {
        ErrorReporter::reportError(
            initializer, "Incompatible types in variable declaration");
        return;
      }
    } else {
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
  for (auto &stmt : expr.getStatements()) {
    stmt->accept(*this);
  }

  // Exit the scope after visiting all statements
  symbolTable.exitScope();
}

DEFINE_VISIT(ClassDeclStmt) {
  // find superclass type if it exists
  std::shared_ptr<ClassType> superClassType = nullptr;
  if (expr.hasSuperclass()) {
    std::shared_ptr<Symbol> superClass =
        symbolTable.lookupSymbol(expr.getSuperclassName());
    if (superClass == nullptr) {
      ErrorReporter::reportError(&expr, "Superclass '" +
                                            expr.getSuperclassName() +
                                            "' is not defined");
      return;
    }
    superClassType = cast<ClassType>(superClass->getType());
  }

  // create a new class type
  std::shared_ptr<ClassType> classType =
      std::make_shared<ClassType>(expr.getName(), superClassType);
  // create a new class symbol
  std::shared_ptr<Symbol> classSymbol =
      std::make_shared<Symbol>(expr.getName(), classType);
  // declare the class symbol in the symbol table
  if (!symbolTable.declare(classSymbol)) {
    ErrorReporter::reportError(&expr, "Class '" + expr.getName() +
                                          "' is already defined");
    return;
  }
  // mark the class symbol as defined
  classSymbol->markAsDefined();

  symbolTable.enterClassScope(expr.getName());
  // visit each field in the class
  for (const auto &field : expr.getFields()) {
    field.second->accept(*this);
    classType->addProperty(field.first,
                           field.second->getType());
  }
  expr.setClassScope(symbolTable.getCurrentScope());
  // exit the class scope
  symbolTable.exitScope();

  if (!classType->hasConstructor()) {
    // if the class does not have a constructor, create a default constructor
    std::vector<std::shared_ptr<Type>> parameters;
    std::shared_ptr<ConstructorType> defaultConstructor =
        std::make_shared<ConstructorType>(classSymbol->getName(),
                                          parameters, classType->getInstanceType());
    classType->addProperty(classSymbol->getName(), defaultConstructor);
  }
}

DEFINE_VISIT(FunctionDecl) {
  std::shared_ptr<Scope> currentScope = symbolTable.getCurrentScope();

  bool isConstructor = currentScope->isInClassScope() &&
                       expr.getName() == currentScope->getCurrentClassSymbol()
                                                    ->getName();

  // create a new function type
  std::vector<std::shared_ptr<Type>> parameterTypes;

  // add parameters to the function type
  for (const auto &param : expr.getParameters()) {
    assert(param->getType() != nullptr && "Parameter type should not be null");
    parameterTypes.push_back(param->getType());
  }

  // check if the function is already defined
  std::shared_ptr<Symbol> funcSymbol =
      symbolTable.lookupLocalSymbol(expr.getName());
  std::shared_ptr<FunctionType> funcType = nullptr;
  bool isOverloaded = true;
  if (funcSymbol == nullptr) {
    // if the function is not defined, create a new function symbol
    if (isConstructor) {
      // funcType = std::make_shared<ConstructorType>(
      //     expr.getName(), parameterTypes, cast<ClassType>(currentScope->getCurrentClassSymbol()->getType())->getInstanceType());
      funcType = std::make_shared<ConstructorType>(expr.getName());
    }
    else {
      funcType = std::make_shared<FunctionType>(expr.getName());
    }
    funcSymbol = std::make_shared<Symbol>(
        expr.getName(), funcType);
    isOverloaded = false;
  }
  else {
    funcType = cast<FunctionType>(funcSymbol->getType());
  }

  // declare the function symbol in the symbol table
  if (!isOverloaded) {
    symbolTable.declare(funcSymbol);
  }
  // mark the function symbol as defined
  funcSymbol->markAsDefined();

  // enter a new function scope
  symbolTable.enterFunctionScope(expr.getName());
  std::shared_ptr<Scope> functionScope = symbolTable.getCurrentScope();
  // visit the function body
  for (auto &statement : expr.getBody()->getStatements()) {
    statement->accept(*this);
  }

  // get return type of the function
  std::shared_ptr<Type> returnType =
      functionScope->getCurrentReturnType();

  if (isConstructor) {
    // if the function is a constructor, set the return type as the class type
    assert(currentScope->getCurrentClassSymbol() != nullptr &&
           "Current class symbol should not be null");
    returnType = cast<ClassType>(currentScope->getCurrentClassSymbol()->getType())->getInstanceType();
  }
  else if (returnType == nullptr) {
    // if the function does not have a return type, set it as nil
    returnType = NilType::getInstance();
    functionScope->setCurrentReturnType(returnType);
  }

  // exit the function scope
  symbolTable.exitScope();

  // add overload to the existing function symbol
  assert(returnType != nullptr && "Return type should not be null");
  FunctionType::Signature signature(parameterTypes, returnType);
  assert(funcType != nullptr && "Function type should not be null");
  // check if the function has the same number of parameters
  if (funcType->hasOverload(signature)) {
    ErrorReporter::reportError(
        &expr, "Function '" + expr.getName() +
                  "' is already defined with the same signature");
    return;
  }
  // add the overload to the function type
  funcType->addOverload(signature);
  expr.setSymbol(funcSymbol);
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
  if (!conditionType->isCompatible(BoolType::getInstance())) {
    ErrorReporter::reportError(
        &expr, "Condition of 'if' statement must be a boolean expression");
    return;
  }

  // resolve the then and else branches
  expr.getThenBranch()->accept(*this);
  if (expr.hasElseBranch()) {
    expr.getElseBranch()->accept(*this);
  }

  // set the type of the if statement as nil
  // expr.setType(NilType::getInstance());
}

DEFINE_VISIT(WhileStmt) {
  // expr.getCondition()->accept(*this);
  // expr.getBody()->accept(*this);

  assert_not_reached("Unimplemented WhileStmt visit");
}

DEFINE_VISIT(ForStmt) { assert_not_reached("Unimplemented ForStmt visit"); }

DEFINE_VISIT(ReturnStmt) {
  // check if the return statement is inside a function scope
  std::shared_ptr<Scope> currentScope = symbolTable.getCurrentScope();
  if (!currentScope->isInFunctionScope()) {
    ErrorReporter::reportError(&expr, "'return' can only be used inside a function");
    return;
  }

  // resolve the return value expression
  ExprBase *returnValue = expr.getValue();
  returnValue->accept(*this);
  std::shared_ptr<Type> returnType = returnValue->getType();
  assert(returnType != nullptr && "Return type should not be null");
  std::shared_ptr<Type> funcReturnType = currentScope->getCurrentReturnType();
  if (funcReturnType == nullptr) {
    currentScope->setCurrentReturnType(returnType);
  }
  else if (!funcReturnType->isCompatible(returnType)) {
    // check if the return type is compatible with the function return type
    std::ostringstream oss;
    oss << "Return type '" << returnType
        << "' is not compatible with function return type '"
        << funcReturnType << "'";
    ErrorReporter::reportError(&expr, oss.str());
    return;
  }
}

// Expression visitors
DEFINE_VISIT(ThisExpr) {
  std::shared_ptr<Scope> currentScope = symbolTable.getCurrentScope();
  if (!currentScope->isInClassScope() || !currentScope->isInFunctionScope()) {
    ErrorReporter::reportError(
        &expr, "'this' can only be used inside a class function");
    return;
  }
  assert(currentScope->getCurrentClassSymbol() != nullptr &&
         "Current class symbol should not be null");
  expr.setType(currentScope->getCurrentClassSymbol()->getType());
}

DEFINE_VISIT(SuperExpr) { assert_not_reached("Unimplemented SuperExpr visit"); }

DEFINE_VISIT(GroupingExpr) {
  expr.getExpression()->accept(*this);
  // [Type inference] set the type of the grouping expression as the type of the
  // inner expression
  std::shared_ptr<Type> innerType = expr.getExpression()->getType();
  if (innerType == nullptr) {
    // [Type inference] if the inner expression type is unknown, infer it as an
    // unresolved type
    innerType = UnresolvedType::getInstance();
    expr.getExpression()->setType(innerType);
  } else {
    // [Type inference] otherwise, set the grouping expression type as the inner
    // expression type
    expr.setType(innerType);
  }
}

DEFINE_VISIT(CallExpr) {
  ExprBase *callee = expr.getCallee();
  assert(callee != nullptr && "Callee should not be null");
  assert((isa<VariableExpr, AccessExpr>(callee)) &&
         "Callee should be a variable, access expression");

  // resolve the callee
  callee->accept(*this);

  std::shared_ptr<FunctionType> calleeType = nullptr;
  // [Type inference] set the type of the call expression as the return type of
  // the callee
  assert(callee->getType() != nullptr &&
         "Callee type should not be null");
  if (isa<FunctionType>(callee->getType())) {
    // if the callee is a function, set the call expression type as the return
    // type of the function
    calleeType = dyn_cast<FunctionType>(callee->getType());
  } else if (isa<ClassType>(callee->getType()) && !isa<InstanceType>(callee->getType())) {
    // if the callee is a class, set the call expression type as the class type
    std::shared_ptr<ClassType> classType =
        dyn_cast<ClassType>(callee->getType());
    assert(classType->hasConstructor() &&
           "Class type should have a constructor");
    calleeType = classType->getConstructor();
  } else {
    ErrorReporter::reportError(
        &expr, "Callee type should be a function type or class type");
    return;
  }

  // resolve the arguments
  std::vector<std::shared_ptr<Type>> argumentTypes;
  for (size_t i = 0; i < expr.getArguments().size(); ++i) {
    auto &arg = expr.getArguments()[i];
    arg->accept(*this);

    // check if the argument type is compatible with the callee type
    std::shared_ptr<Type> argType = arg->getType();
    if (argType == nullptr) {
      // [Type inference] if the argument type is unknown, infer it as an
      // unresolved type
      argumentTypes.push_back(UnresolvedType::getInstance());
    }
    // special case for 'print' function
    else if (isa<ClassType>(argType) && calleeType->getName() == "print") {
      // [Type inference] if the argument is a class type and the callee is
      // 'print', we can treat it as a string type
      argumentTypes.push_back(StringType::getInstance());
    }

    else {
      // [Type inference] otherwise, add the argument type to the list of
      // argument types
      argumentTypes.push_back(argType);
    }
  }
  std::shared_ptr<Type> calleeReturnType = calleeType->getReturnType();
  FunctionType::Signature calleeSignature(argumentTypes, calleeReturnType);
  if (calleeType->hasOverload(calleeSignature)) {
    // if the callee has an overload with the same signature, set the call
    // expression type as the return type of the overload
    expr.setCalleeSignature(calleeSignature);
  } else {
    std::ostringstream oss;
    oss << "No matching overload for function '";
    if (isa<VariableExpr>(callee)) {
      oss << dyn_cast<VariableExpr>(callee)->getName();
    } else if (isa<AccessExpr>(callee)) {
      oss << dyn_cast<AccessExpr>(callee)->getProperty();
    } else {
      assert_not_reached("Callee should be a variable or access expression");
    }
    calleeSignature.print(oss);
    oss << "'";
    ErrorReporter::reportError(&expr, oss.str());
    return;
  }

  expr.setType(calleeReturnType);
}

DEFINE_VISIT(VariableExpr) {
  // check if the variable is declared
  std::shared_ptr<Symbol> symbol = symbolTable.lookupSymbol(expr.getName());
  if (symbol == nullptr) {
    ErrorReporter::reportError(&expr, "Use of Undeclared Variable '" +
                                          expr.getName() + "'");
    return;
  }

  // set the type of the variable as the symbol type
  expr.setSymbol(symbol.get());

  if (!symbol->isDefinedSymbol()) {
    // ErrorReporter::reportError(&expr, "Variable '" + expr.getName() + "' is
    // used before it is defined");
    return;
  }
  if (!symbol->isUsedSymbol()) {
    // mark the symbol as used
    symbol->markAsUsed();
  }
}

DEFINE_VISIT(LiteralExpr) {
  const std::string &value = expr.getValue();
  if (value.empty()) {
    // [Type inference] if the literal is empty, infer it as an unresolved type
    expr.setType(UnresolvedType::getInstance());
  } else if (value == "true" || value == "false") {
    // [Type inference] if the literal is a boolean, set the type as BoolType
    expr.setType(BoolType::getInstance());
  } else if (value == "nil") {
    // [Type inference] if the literal is nil, set the type as NilType
    expr.setType(NilType::getInstance());
  } else if (std::all_of(value.begin(), value.end(), ::isdigit)) {
    // [Type inference] if the literal is a number, set the type as NumberType
    expr.setType(NumberType::getInstance());
  } else if (value.front() == '"' && value.back() == '"') {
    // [Type inference] if the literal is a string, set the type as StringType
    expr.setType(StringType::getInstance());
  } else {
    ErrorReporter::reportError(&expr, "Unknown literal type: '" + value + "'");
  }
}

DEFINE_VISIT(NumberExpr) {
  // [Type inference] set the type of the number expression as NumberType
  expr.setType(NumberType::getInstance());
}

DEFINE_VISIT(StringExpr) { expr.setType(StringType::getInstance()); }

DEFINE_VISIT(UnaryExpr) {
  // resolve the right expression
  expr.getRight()->accept(*this);

  // check if the right type is compatible with the unary operator
  std::shared_ptr<Type> rightType = expr.getRight()->getType();
  if (rightType == nullptr) {
    // [Type inference] if the right type is unknown, infer it as an unresolved
    // type
    rightType = UnresolvedType::getInstance();
    expr.getRight()->setType(rightType);
  } else if (!isa<NumberType, BoolType, NilType, StringType>(rightType.get())) {
    // [Type inference] if the right type is not a number, boolean or nil,
    // report an error
    ErrorReporter::reportError(
        &expr, "Invalid type for unary operator '" +
                   convertTokenTypeToString(expr.getOpKind()) + "'");
    return;
  }

  // set the result type of the unary expression as the right type
  expr.setType(BoolType::getInstance());
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
      ErrorReporter::reportError(&expr,
                                 "Incompatible types in binary expression");
      return;
    }
  } else if (leftType == nullptr && rightType != nullptr) {
    // [Type inference] if the left type is unknown, infer left as the right
    // type
    expr.getLeft()->setType(rightType);
  } else if (leftType != nullptr && rightType == nullptr) {
    // [Type inference] if the right type is unknown, infer right as the left
    // type
    expr.getRight()->setType(leftType);
  } else {
    ErrorReporter::reportError(
        &expr, "Cannot perform operation on expressions of unknown type");
  }

  // set the result type of the binary expression as the left type
  expr.setType(leftType);
}

DEFINE_VISIT(AccessExpr) {
    ExprBase *base = expr.getBase();
    base->accept(*this);

    const std::shared_ptr<Type> baseType = base->getType();
    assert(baseType != nullptr && "Base type should not be null in AccessExpr");
    if (auto classType = dyn_cast<ClassType>(baseType)) {
      const std::shared_ptr<Type> propertyType =
          classType->getPropertyType(expr.getProperty());
      expr.setType(propertyType);
    }
    else {
      ErrorReporter::reportError(&expr,
                                 "Base expression must be a class or function type");
      return;
    }
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
  } else if (leftType == nullptr && rightType != nullptr) {
    // [Type inference] if the left type is unknown, infer left as the right
    // type
    expr.getLeft()->setType(rightType);
  } else if (leftType != nullptr && rightType == nullptr) {
    // [Type inference] if the right type is unknown, infer right as the left
    // type
    expr.getRight()->setType(leftType);
  } else {
    ErrorReporter::reportError(&expr,
                               "Cannot assign an expression of unknown type");
    return;
  }

  // set the result type of the assignment as the left type
  expr.setType(leftType);

  if (VariableExpr *var = dyn_cast<VariableExpr>(expr.getLeft())) {
    if (var->getSymbol() == nullptr) {
      ErrorReporter::reportError(&expr, "Variable '" + var->getName() +
                                            "' is not declared");
      return;
    }
    var->getSymbol()->markAsDefined();
  }
}

#undef DEFINE_VISIT

} // namespace lox
