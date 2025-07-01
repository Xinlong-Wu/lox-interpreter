#include "Compiler/Sema/TypeSystem/TypeInfer/TypeInferenceEngine.h"
#include "Common.h"
#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/AST/ASTWalker.h"
#include "Compiler/Sema/Scope.h"
#include "Compiler/Sema/TypeSystem/Type.h"

#include <iostream>
#include <optional>

using namespace std;
using namespace lox;

lox::TypeInferenceEngine::TypeInferenceEngine(TypeContext *typeContext)
    : typeContext(typeContext), symbolTable(typeContext) {}

void lox::TypeInferenceEngine::inferProgramTypes(
    const vector<unique_ptr<StmtBase>> &statements) {
  // 收集类型声明
  collectTypeDeclarations(statements);

  // 推断类型
  inferStatements(statements);

  // 解决约束
  bool success = solveConstraints();
  if (!success) {
    // // 再次推断类型
    // inferStatements(statements);
    // solveConstraints();
    return;
  }

  // 应用类型替换
  applySubstitutions(statements);
}

void lox::TypeInferenceEngine::collectTypeDeclarations(
    const vector<unique_ptr<StmtBase>> &statements) {
  for (const auto &stmt : statements) {
    if (auto classDecl = dyn_cast<ClassDeclStmt>(stmt.get())) {
      collectClassDeclarations(classDecl);
    } else if (auto funcDecl = dyn_cast<FunctionDeclStmt>(stmt.get())) {
      collectFunctionDeclarations(funcDecl);
    } else if (auto blockStmt = dyn_cast<BlockStmt>(stmt.get())) {
      // enter a new scope for the block statement
      unique_ptr<BlockScope> blockScope =
          make_unique<BlockScope>(symbolTable.currentScope());
      symbolTable.enterScope(blockScope.get());
      blockStmt->setScope(move(blockScope));
      // 处理块语句中的声明
      collectTypeDeclarations(blockStmt->getStatements());

      // exit the block scope
      symbolTable.exitScope();
    }
  }
}

void lox::TypeInferenceEngine::collectClassDeclarations(
    ClassDeclStmt *classDecl) {
  ClassType *superClass = nullptr;

  if (classDecl->hasSuperclass()) {
    superClass = dyn_cast<ClassType>(
        symbolTable.lookupType(classDecl->getSuperclassName()));
    if (!superClass) {
      ErrorReporter::reportError("Superclass '" +
                                 classDecl->getSuperclassName() +
                                 "' is not defined.");
      return;
    }
  }

  string className = classDecl->getName();
  ClassType *classType = typeContext->make<ClassType>(className, superClass);

  if (symbolTable.lookupTypeLocal(className) ||
      symbolTable.lookupLocalSymbol(className)) {
    ErrorReporter::reportError("Class '" + classDecl->getName() +
                               "' already declared");
    return;
  }

  symbolTable.declare(make_unique<Symbol>(className, classType));
  symbolTable.declareType(className, classType);

  unique_ptr<ClassScope> classScope =
      make_unique<ClassScope>(symbolTable.currentScope(), className);
  ClassScope *classScopePtr = classScope.get();
  classDecl->setScope(move(classScope));
  classType->setClassScope(classScopePtr);
  symbolTable.enterScope(classScopePtr);

  for (auto &function : classDecl->getMethods()) {
    this->collectFunctionDeclarations(function.second.get());
  }

  const Symbol *constructorSymbol = classScopePtr->getConstructor();

  if (!constructorSymbol) {
    // If the class does not have a constructor, create a default constructor
    unique_ptr<Signature> signature =
        make_unique<Signature>(vector<Type *>(), classType->getInstanceType());
    FunctionType *funcType = typeContext->make<FunctionType>(
        classType->getConstructorName(), std::move(signature));

    classScopePtr->declare(std::move(make_unique<Symbol>(funcType)));
  }

  symbolTable.exitScope();
}

void lox::TypeInferenceEngine::collectFunctionDeclarations(
    FunctionDeclStmt *funcDecl) {
  // collect the function's parameters type
  vector<Type *> paramTypes;
  vector<unique_ptr<Symbol>> paramSymbols;
  for (const auto &param : funcDecl->getParameters()) {
    Type *paramType;
    if (param->getTypeAnnotation() != std::nullopt) {
      auto type = symbolTable.lookupType(*param->getTypeAnnotation());
      if (!type) {
        ErrorReporter::reportError("Type '" + *param->getTypeAnnotation() +
                                   "' not found for parameter '" +
                                   param->getName() + "'");
        return;
      }
      paramType = type;
    } else {
      paramType = TypeVariable::create();
    }
    param->setType(paramType);
    paramTypes.push_back(paramType);
    paramSymbols.push_back(make_unique<Symbol>(param->getName(), paramType));
  }

  // collect the function's return type
  Type *returnType = TypeVariable::create();
  // check function is a constructor
  ClassType *classTy = symbolTable.currentScope()->getCurrentClassType();
  if (classTy && funcDecl->getName() == classTy->getConstructorName()) {
    // if the function is a constructor, the return type is the instance type
    returnType = classTy->getInstanceType();
  }

  unique_ptr<Signature> signature =
      make_unique<Signature>(std::move(paramTypes), returnType);
  Signature *signaturePtr = signature.get();
  funcDecl->setSignature(signaturePtr);

  // check if the function is overloaded
  Symbol *overloadedFunc = symbolTable.lookupLocalSymbol(funcDecl->getName());
  FunctionType *funcType = nullptr;
  if (overloadedFunc) {
    FunctionType *existingFuncTypePtr =
        dyn_cast<FunctionType>(overloadedFunc->getType());
    if (!existingFuncTypePtr) {
      ErrorReporter::reportError("Symbol '" + funcDecl->getName() +
                                 "' is not a function");
      return;
    }
    existingFuncTypePtr->addOverload(std::move(signature));
    funcDecl->setType(existingFuncTypePtr);
  } else {
    // create a new function type and declare it
    funcType = typeContext->make<FunctionType>(funcDecl->getName(),
                                               std::move(signature));
    if (!symbolTable.declare(std::move(make_unique<Symbol>(funcType)))) {
      ErrorReporter::reportError("Function '" + funcDecl->getName() +
                                 "' already declared");
      return;
    }

    funcDecl->setType(funcType);
    if (!symbolTable.declareType(funcDecl->getName(), std::move(funcType))) {
      ErrorReporter::reportError("Function type '" + funcDecl->getName() +
                                 "' already declared");
      return;
    }
  }

  // enter function scope
  unique_ptr<FunctionScope> funcScope = make_unique<FunctionScope>(
      symbolTable.currentScope(), funcDecl->getName(), signaturePtr);
  symbolTable.enterScope(funcScope.get());
  FunctionScope *funcScopePtr = funcScope.get();
  funcDecl->setScope(move(funcScope));

  // declare the function's parameters in the function scope
  for (auto paramSymbol = paramSymbols.begin();
       paramSymbol != paramSymbols.end(); ++paramSymbol) {
    if (!funcScopePtr->declare(std::move(*paramSymbol))) {
      ErrorReporter::reportError("Parameter '" + (*paramSymbol)->getName() +
                                 "' already declared in function '" +
                                 funcDecl->getName() + "'");
      return;
    }
  }

  // collect the function's body statements
  collectTypeDeclarations(funcDecl->getBody()->getStatements());

  // exit function scope
  symbolTable.exitScope();
}

void lox::TypeInferenceEngine::inferStatements(
    const vector<unique_ptr<StmtBase>> &statements) {
  for (const auto &stmt : statements) {
    inferStatement(stmt.get());
  }
}

void lox::TypeInferenceEngine::inferStatement(StmtBase *stmt) {
  if (auto varDecl = dyn_cast<VarDeclStmt>(stmt)) {
    inferVarDeclStmt(varDecl);
  } else if (auto funcDecl = dyn_cast<FunctionDeclStmt>(stmt)) {
    inferFunctionDeclStmt(funcDecl);
  } else if (auto classDecl = dyn_cast<ClassDeclStmt>(stmt)) {
    inferClassDeclStmt(classDecl);
  } else if (auto blockStmt = dyn_cast<BlockStmt>(stmt)) {
    inferBlockStmt(blockStmt);
  } else if (auto exprStmt = dyn_cast<ExpressionStmt>(stmt)) {
    inferExprStmt(exprStmt);
  } else if (auto retureStmt = dyn_cast<ReturnStmt>(stmt)) {
    inferReturnStmt(retureStmt);
  }
}

void lox::TypeInferenceEngine::inferVarDeclStmt(VarDeclStmt *varDecl) {
  Type *varType;

  // check if varDecl has an initializer
  if (varDecl->getInitializer()) {
    Type *initType = inferExpr(varDecl->getInitializer());

    optional<string> typeAnnotation = varDecl->getTypeAnnotation();
    if (typeAnnotation) {
      Type *declaredType = symbolTable.lookupType(*typeAnnotation);
      if (!declaredType) {
        ErrorReporter::reportError("Type '" + *typeAnnotation +
                                   "' not found for variable '" +
                                   varDecl->getName() + "'");
        return;
      }

      // add a constraint between the initializer type and the declared type
      addConstraint(initType, declaredType,
                    Constraint::ConstraintType::ASSIGNABLE);
      varType = declaredType;
    } else {
      // if no type is declared, we can infer the type from the initializer
      varType = initType;
    }
  } else {
    // if no initializer, we check if a type is declared
    optional<string> typeAnnotation = varDecl->getTypeAnnotation();
    if (typeAnnotation) {
      Type *declaredType = symbolTable.lookupType(*typeAnnotation);
      if (!declaredType) {
        ErrorReporter::reportError("Type '" + *typeAnnotation +
                                   "' not found for variable '" +
                                   varDecl->getName() + "'");
        return;
      }

      varType = declaredType;
    } else {
      // if no initializer and no type declared, we use a type variable
      varType = TypeVariable::create();
    }
  }

  if (!symbolTable.declare(make_unique<Symbol>(varDecl->getName(), varType))) {
    ErrorReporter::reportError("Variable '" + varDecl->getName() +
                               "' already declared");
    return;
  }
  varDecl->setType(varType);
}

void lox::TypeInferenceEngine::inferFunctionDeclStmt(
    FunctionDeclStmt *funcDecl) {
  // restore the function scope to the symbol table
  FunctionScope *funcScope = cast<FunctionScope>(funcDecl->getScope());
  if (!funcScope) {
    ErrorReporter::reportError("Function '" + funcDecl->getName() +
                               "' has no scope");
    return;
  }
  symbolTable.enterScope(funcScope);

  inferStatements(funcDecl->getBody()->getStatements());

  symbolTable.exitScope();
}

void lox::TypeInferenceEngine::inferClassDeclStmt(ClassDeclStmt *classDecl) {
  // class has a scope means it is failed in Type Collection
  if (!classDecl->getScope()) {
    return;
  }
  // restore the class scope to the symbol table
  ClassScope *classScope = cast<ClassScope>(classDecl->getScope());
  symbolTable.enterScope(classScope);

  // infer the class's fields
  for (const auto &field : classDecl->getFields()) {
    inferStatement(field.second.get());
  }

  // infer the class's methods
  for (const auto &method : classDecl->getMethods()) {
    inferFunctionDeclStmt(method.second.get());
  }

  symbolTable.exitScope();
}

void lox::TypeInferenceEngine::inferReturnStmt(ReturnStmt *returnStmt) {
  // check if the return statement is in a function scope
  if (!symbolTable.currentScope()->inFunctionScope()) {
    ErrorReporter::reportError(
        "Return statement chouldn't outside of function scope");
    return;
  }

  // infer the return expression type
  Type *returnType = nullptr;
  if (returnStmt->getValue()) {
    returnType = inferExpr(returnStmt->getValue());
    if (!returnType) {
      ErrorReporter::reportError("Return expression has no type");
      return;
    }
  }

  // get the current function's signature
  FunctionScope *funcScope = cast<FunctionScope>(
      symbolTable.currentScope()->getCurrentFunctionScope());
  const Signature *signature = funcScope->getSignature();
  if (!signature) {
    ErrorReporter::reportError("No current function signature found");
    return;
  }

  // add a constraint between the return type and the function's return type
  addConstraint(returnType, signature->getReturnType(),
                Constraint::ConstraintType::ASSIGNABLE);
}

void lox::TypeInferenceEngine::inferBlockStmt(BlockStmt *blockStmt) {
  // restore the block scope to the symbol table
  BlockScope *blockScope = cast<BlockScope>(blockStmt->getScope());
  symbolTable.enterScope(blockScope);

  // infer the block's statements
  inferStatements(blockStmt->getStatements());

  symbolTable.exitScope();
}

void lox::TypeInferenceEngine::inferExprStmt(ExpressionStmt *exprStmt) {
  // infer the expression in the statement
  Type *exprType = inferExpr(exprStmt->getExpression());
  if (!exprType) {
    ErrorReporter::reportError("Expression in statement has no type");
    return;
  }
}

Type *lox::TypeInferenceEngine::inferExpr(ExprBase *expr, Type *expectedType) {
  Type *inferredType = nullptr;
  if (isa<NumberExpr>(expr)) {
    return typeContext->getNumberType();
  }
  if (isa<StringExpr>(expr)) {
    return typeContext->getStringType();
  }
  if (isa<BoolExpr>(expr)) {
    return typeContext->getBoolType();
  }
  if (isa<NilExpr>(expr)) {
    return typeContext->getNilType();
  }
  if (auto varExpr = dyn_cast<IdentifierExpr>(expr)) {
    if (varExpr->isThis()) {
      // 'this' refers to the current instance in a class context
      if (auto classType = symbolTable.currentScope()->getCurrentClassType()) {
        return classType;
      } else {
        ErrorReporter::reportError(
            "'this' can only be used inside a class method");
        return nullptr;
      }
    }

    if (varExpr->isSuper()) {
      if (auto classType = symbolTable.currentScope()->getCurrentClassType()) {
        // 'super' refers to the superclass of the current class
        ClassType *superClassType = classType->getSuperClass();
        if (superClassType) {
          return superClassType;
        } else {
          ErrorReporter::reportError("'super' can only be used in a subclass");
          return nullptr;
        }
      } else {
        ErrorReporter::reportError(
            "'super' can only be used inside a class method");
        return nullptr;
      }
    }

    Symbol *symbol = symbolTable.lookupSymbol(varExpr->getName());
    if (!symbol) {
      ErrorReporter::reportError("Use of undeclared variable '" +
                                 varExpr->getName() + "'");
      return nullptr;
    }

    inferredType = symbol->getType();
  }
  if (auto binaryExpr = dyn_cast<BinaryExpr>(expr)) {
    inferredType = inferBinaryExpr(binaryExpr, expectedType);
  }
  if (auto unaryExpr = dyn_cast<UnaryExpr>(expr)) {
    inferredType = inferUnaryExpr(unaryExpr, expectedType);
  }
  if (auto callExpr = dyn_cast<CallExpr>(expr)) {
    inferredType = inferCallExpr(callExpr, expectedType);
  }
  if (auto assignExpr = dyn_cast<AssignExpr>(expr)) {
    inferredType = inferAssignExpr(assignExpr, expectedType);
  }
  if (auto accessExpr = dyn_cast<AccessExpr>(expr)) {
    inferredType = inferAccessExpr(accessExpr, expectedType);
  }

  if (isa<TypeVariable>(inferredType)) {
    // Apply substitution to type variables
    inferredType = applySubstitution(inferredType);
  }
  return inferredType;
}

Type *lox::TypeInferenceEngine::inferBinaryExpr(BinaryExpr *binaryExpr,
                                                Type *expectedType) {
  Type *leftType = inferExpr(binaryExpr->getLeft());
  Type *rightType = inferExpr(binaryExpr->getRight());

  Type *resultType = nullptr;

  // Add constraints based on the operation
  switch (binaryExpr->getOp()) {
  case BinaryExpr::Op::Add:
  case BinaryExpr::Op::Sub:
  case BinaryExpr::Op::Mul:
  case BinaryExpr::Op::Div:
    // self.add_constraint(left_type, right_type)
    addConstraint(leftType, rightType, Constraint::ConstraintType::EQUAL);
    if (expectedType) {
      addConstraint(leftType, expectedType, Constraint::ConstraintType::EQUAL);
    }
    resultType = leftType;
    break;
  case BinaryExpr::Op::Or:
  case BinaryExpr::Op::Equal:
  case BinaryExpr::Op::NotEqual:
  case BinaryExpr::Op::GreaterThan:
  case BinaryExpr::Op::GreaterThanOrEqual:
    addConstraint(leftType, rightType, Constraint::ConstraintType::EQUAL);
    resultType = typeContext->getBoolType();
    break;
  default:
    ErrorReporter::reportError("Unknown binary operation: " +
                               BinaryExpr::toString(binaryExpr->getOp()));
    return nullptr;
  }

  binaryExpr->setType(resultType);
  return resultType;
}

Type *lox::TypeInferenceEngine::inferUnaryExpr(UnaryExpr *unaryExpr,
                                               const Type *expectedType) {
  Type *operandType = inferExpr(unaryExpr->getOperand());

  Type *resultType = nullptr;

  switch (unaryExpr->getOp()) {
  case UnaryExpr::Op::Negate:
    resultType = typeContext->getNumberType();
    break;
  case UnaryExpr::Op::Not:
    resultType = typeContext->getBoolType();
    break;
  default:
    ErrorReporter::reportError("Unknown unary operation: " +
                               UnaryExpr::toString(unaryExpr->getOp()));
    return nullptr;
  }
  addConstraint(operandType, resultType, Constraint::ConstraintType::EQUAL);

  unaryExpr->setType(resultType);
  return resultType;
}

Type *lox::TypeInferenceEngine::inferCallExpr(CallExpr *callExpr,
                                              Type *expectedType) {
  Type *resultType = nullptr;

  vector<Type *> argTypes;
  for (size_t i = 0; i < callExpr->getArgumentCount(); ++i) {
    Type *argType = inferExpr(callExpr->getArgument(i));
    if (!argType) {
      ErrorReporter::reportError("Argument " + std::to_string(i) +
                                 " has no type");
      return nullptr;
    }

    if (isa<TypeVariable>(argType)) {
      // Apply substitution to type variables
      argType = applySubstitution(argType);
    }

    argTypes.push_back(argType);
  }

  Type *calleeType = inferExpr(callExpr->getCallee());

  vector<const Signature *> bestMatches;
  const FunctionType *functionType = dyn_cast<const FunctionType>(calleeType);

  if (!functionType) {
    if (auto classType = dyn_cast<ClassType>(calleeType)) {
      // if the callee is a class type, we assume it's a constructor call
      const ClassScope *classScope = classType->getClassScope();
      functionType =
          cast<const FunctionType>(classScope->getConstructor()->getType());
    } else if (isa<InstenceType>(calleeType)) {
      ErrorReporter::reportError("Unable call '" + calleeType->getName() +
                                 "' as a function");
      return nullptr;
    } else {
      ErrorReporter::reportError("Callee is not a function type");
      return nullptr;
    }
  }

  bestMatches = functionType->resolveOverload(argTypes, typeContext);

  if (bestMatches.empty()) {
    ErrorReporter::reportError("No matching function overload found for call");
    return nullptr;
  }

  if (bestMatches.size() == 1) {
    const Signature *bestMatch = bestMatches[0];
    // add constraints for each argument
    for (size_t i = 0; i < argTypes.size(); ++i) {
      addConstraint(argTypes[i], bestMatch->getParameterType(i),
                    Constraint::ConstraintType::ASSIGNABLE);
    }
    // set the return type of the call expression
    resultType = bestMatch->getReturnType();
    if (expectedType) {
      addConstraint(expectedType, resultType,
                    Constraint::ConstraintType::ASSIGNABLE);
    }
    callExpr->setResolvedSignature(bestMatch);
    callExpr->setType(resultType);
  } else {

    // TODO: calculate most generic signature from best matches
    assert_not_reached(
        "Unimplemented: multiple best matches for call expression");
  }

  return resultType;
}

Type *lox::TypeInferenceEngine::inferAssignExpr(AssignExpr *assignExpr,
                                                Type *expectedType) {
  Type *targetType = inferExpr(assignExpr->getTarget());
  Type *valueType = inferExpr(assignExpr->getValue());

  if (!targetType) {
    ErrorReporter::reportError("Target of assignment has no type");
    return nullptr;
  }

  // add a constraint between the target type and the value type
  addConstraint(valueType, targetType, Constraint::ConstraintType::ASSIGNABLE);
  assignExpr->setType(targetType);
  return targetType;
}

Type *lox::TypeInferenceEngine::inferAccessExpr(AccessExpr *accessExpr,
                                                const Type *expectedType) {
  Type *objectType = inferExpr(accessExpr->getObject());
  if (isa<TypeVariable>(objectType)) {
    objectType = applySubstitution(objectType);
  }

  const string &fieldName = accessExpr->getFieldName();
  Type *fieldType = nullptr;
  if (auto instenceType = dyn_cast<const InstenceType>(objectType)) {
    // check if the field exists in the class
    fieldType = instenceType->getPropertyType(fieldName);
  } else if (auto classType = dyn_cast<const ClassType>(objectType)) {
    fieldType = classType->getStaticPropertyType(fieldName);
  } else {
    ostringstream ss;
    ss << "Unable to infer access expression '";
    accessExpr->print(ss);
    ErrorReporter::reportError(ss.str());
    return nullptr;
  }

  if (!fieldType) {
    ErrorReporter::reportError("Field '" + fieldName + "' not found in '" +
                               objectType->getName() + "'");
    return nullptr;
  }
  accessExpr->setType(fieldType);
  return fieldType;
}

bool lox::TypeInferenceEngine::solveConstraint(
    Type *left, Type *right, Constraint::ConstraintType relation,
    bool reportError) {
  switch (relation) {
  case Constraint::ConstraintType::EQUAL: {
    bool success = false;
    success = unify(left, right);
    if (!success && reportError) {
      ErrorReporter::reportError("Cannot unify different types: '" +
                                 left->getName() + "' and '" +
                                 right->getName() + "'");
    }
    return success;
  }
  case Constraint::ConstraintType::ASSIGNABLE: {
    bool success = false;
    if (isa<TypeVariable>(left) || isa<TypeVariable>(right)) {
      // If either side is a type variable, we can unify them
      success = unify(left, right);
      if (!success && reportError) {
        ErrorReporter::reportError("Cannot unify different class types: '" +
                                   left->getName() + "' and '" +
                                   right->getName() + "'");
      }
      return success;
    }
    // Otherwise, we check if left is assignable to right
    success = assinable(left, right);
    if (!success && reportError) {
      ErrorReporter::reportError("Cannot assign type '" + left->getName() +
                                 "' to type '" + right->getName() + "'");
    }
    return success;
  }
  default:
    ErrorReporter::reportError("Unknown constraint relation");
    return false;
  }
  assert_not_reached("Unreachable code in solveConstraint");
  return false;
}

bool lox::TypeInferenceEngine::solveConstraints() {
  bool inferSuccess = true;
  for (const auto &constraint : constraints) {
    inferSuccess &=
        solveConstraint(constraint.getLeftType(), constraint.getRightType(),
                        constraint.getRelation(), true);
  }
  return inferSuccess;
}

bool lox::TypeInferenceEngine::unify(Type *left, Type *right) {
  Type *leftType = applySubstitution(left);
  Type *rightType = applySubstitution(right);

  if (leftType == rightType) {
    return true; // already unified
  }

  if (isa<TypeVariable>(leftType)) {
    if (occursCheck(cast<TypeVariable>(leftType), rightType)) {
      ErrorReporter::reportError("Type variable '" + leftType->getName() +
                                 "' occurs in '" + rightType->getName() + "'");
      return false;
    }
    substitutions[cast<TypeVariable>(leftType)] = rightType;
    return true;
  }

  if (isa<TypeVariable>(rightType)) {
    if (occursCheck(cast<TypeVariable>(rightType), leftType)) {
      ErrorReporter::reportError("Type variable '" + rightType->getName() +
                                 "' occurs in '" + leftType->getName() + "'");
      return false;
    }
    substitutions[cast<TypeVariable>(rightType)] = leftType;
    return true;
  }

  if (isa<FunctionType>(leftType) && isa<FunctionType>(rightType)) {
    assert_not_reached("Unimplemented FunctionType unification");
  }

  if (isa<ClassType>(leftType) && isa<ClassType>(rightType)) {
    const ClassType *leftClass = cast<ClassType>(leftType);
    const ClassType *rightClass = cast<ClassType>(rightType);

    if (leftClass != rightClass) {
      return false;
    }
    return true;
  }
  return false;
}

bool lox::TypeInferenceEngine::assinable(Type *left, Type *right) {
  const Type *from = applySubstitution(left);
  const Type *to = applySubstitution(right);

  if (to == typeContext->getStringType() && isa<ClassType>(from)) {
    return true; // Any type can be assigned to any type
  }

  if (!from->isCompatibleWith(to)) {
    return false;
  }
  return true; // Types are assignable
}

bool lox::TypeInferenceEngine::occursCheck(const TypeVariable *var,
                                           Type *type) {
  const Type *subStitutedType = applySubstitution(type);

  if (var == subStitutedType) {
    return true; // Type variable occurs in the type
  }

  // if (auto classType = dyn_cast<const ClassType>(subStitutedType)) {
  //   // Check if the type variable occurs in the class's fields
  //   for (auto &propertyType : classType->getPropertyTypes()) {
  //     if (occursCheck(var, propertyType)) {
  //       return true;
  //     }
  //   }
  // }
  // else if (auto functionType = dyn_cast<FunctionType>(subStitutedType)) {
  //     // Check if the type variable occurs in the function's parameters or
  //     return type for (const auto &param :
  //     functionType->getSignature()->getParameters()) {
  //         if (occursCheck(var, param)) {
  //             return true;
  //         }
  //     }
  //     if (occursCheck(var, functionType->getSignature()->getReturnType()))
  //     {
  //         return true;
  //     }
  // }

  return false; // Type variable does not occur in the type
}

void lox::TypeInferenceEngine::applySubstitutions(
    const vector<unique_ptr<StmtBase>> &statements) {
  lox::Walker astWalker;
  // astWalker.registerCallback<ExprBase>([this](ExprBase *expr) {
  //     if (auto typeVar = dyn_cast<TypeVariable>(expr->getType())) {
  //         expr->setType(applySubstitution(typeVar));
  //     }
  // });

  auto declCallback = [this](Declaration *decl) -> WalkResult {
    if (auto typeVar = dyn_cast<TypeVariable>(decl->getType())) {
      decl->setType(applySubstitution(typeVar));
    }
    return WalkResult::Advance;
  };
  astWalker.registerCallback<FunctionDeclStmt>(declCallback);
  astWalker.registerCallback<ClassDeclStmt>(declCallback);
  astWalker.registerCallback<VarDeclStmt>(declCallback);

  for (auto &stmt : statements) {
    stmt->walk(astWalker);
  }
}

Type *lox::TypeInferenceEngine::applySubstitution(Type *type) {
  if (auto typeVar = dyn_cast<TypeVariable>(type)) {
    auto it = substitutions.find(typeVar);
    if (it != substitutions.end()) {
      return applySubstitution(it->second);
    }
  }
  return type;
}

void lox::TypeInferenceEngine::printConstraints() const {
  std::cout << "ID \t Type 1 \t Constraint \t Type 2" << std::endl;
  for (size_t i = 0; i < constraints.size(); ++i) {
    const auto &constraint = constraints[i];
    std::cout << i << "\t";
    constraint.print(std::cout);
    std::cout << std::endl;
  }
}
