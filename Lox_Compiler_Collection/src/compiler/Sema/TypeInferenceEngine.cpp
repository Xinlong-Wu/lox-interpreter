#include "Compiler/Sema/TypeInferenceEngine.h"

using namespace std;

lox::TypeInferenceEngine::NumberType = make_shared<lox::PrimitiveType>("Number");
lox::TypeInferenceEngine::StringType = make_shared<lox::PrimitiveType>("String");
lox::TypeInferenceEngine::BoolType = make_shared<lox::PrimitiveType>("Bool");
lox::TypeInferenceEngine::NilType = lox::NilType::create();

lox::TypeInferenceEngine::TypeInferenceEngine() {
    // 初始化符号表
    symbolTable = SymbolTable();

    // 添加基本类型到符号表
    symbolTable.declareType("Number", NumberType);
    symbolTable.declareType("String", StringType);
    symbolTable.declareType("Bool", BoolType);
    symbolTable.declareType("Nil", NilType);
}

void lox::TypeInferenceEngine::inferProgramTypes(const vector<unique_ptr<StmtBase>> &statements) {
    // 收集类型声明
    collectTypeDeclarations(statements);

    // 推断类型
    inferStatements(statements);

    // 解决约束
    bool success = solveConstraints();
    if (!success) {
        ErrorReporter::reportError("Type inference failed due to unsolvable constraints");
        return;
    }

    // 应用类型替换
    applySubstitution(statements);
}

void lox::TypeInferenceEngine::collectTypeDeclarations(const vector<unique_ptr<StmtBase>> &statements) {
    for (const auto &stmt : statements) {
        if (auto classDecl = dyn_cast<ClassDeclStmt *>(stmt.get())) {
            collectClassDeclarations(classDecl);
        } else if (auto funcDecl = dyn_cast<FunctionDeclStmt *>(stmt.get())) {
            collectFunctionDeclarations(funcDecl);
        }
        else if (auto blockStmt = dyn_cast<BlockStmt *>(stmt.get())) {
            // enter a new scope for the block statement
            shared_ptr<BlockScope> blockScope = make_shared<BlockScope>(symbolTable.currentScope());
            blockStmt->setScope(blockScope);
            symbolTable.enterScope(blockScope);
            // 处理块语句中的声明
            collectTypeDeclarations(blockStmt->getStatements());

            // exit the block scope
            symbolTable.exitScope();
        }
    }
}

void lox::TypeInferenceEngine::collectClassDeclarations(ClassDeclStmt *classDecl) {
    ClassType *superClass = nullptr;

    if (classDecl->hasSuperclass()) {
        superClass = cast<ClassType>(symbolTable.lookupType(classDecl->getSuperClass()->getName()));
        if (!superClass) {
            ErrorReporter::reportError("Superclass '" + classDecl->getSuperClass()->getName() + "' not found");
            return;
        }
    }

    string className = classDecl->getName();
    shared_ptr<ClassType> classType = make_shared<ClassType>(className, superClass);

    if (!symbolTable.declareType(className, classType)) {
        ErrorReporter::reportError("Class '" + classDecl->getName() + "' already declared");
        return;
    }

    shared_ptr<ClassScope> classScope = make_shared<ClassScope>(symbolTable.currentScope(), className);
    symbolTable.enterScope(classScope);

    for(auto &function : classDecl->getMethods()) {
        this->collectFunctionDeclarations(function.second.get());
    }

    symbolTable.exitScope();
}

void lox::TypeInferenceEngine::collectFunctionDeclarations(FunctionDeclStmt *funcDecl) {
    // collect the function's parameters type
    vector<Type*> paramTypes;
    vector<unique_ptr<Symbol> paramSymbols;
    for(const auto &param : funcDecl->getParameters()) {
        Type* paramType;
        if (param->getTypeAnnotation()) {
            auto type = symbolTable.lookupType(*param->getTypeAnnotation());
            if (!type) {
                ErrorReporter::reportError("Type '" + *param->getTypeAnnotation() + "' not found for parameter '" + param->getName() + "'");
                return;
            }
            paramType = type;
        } else {
            paramType = TypeVariable::create();
        }
        paramTypes.push_back(paramType);
        paramSymbols.push_back(make_unique<Symbol>(param->getName(), paramType));
    }

    // collect the function's return type
    Type* returnType = TypeVariable::create();
    funcDecl->setSignature(make_shared<FunctionType::Signature>(paramTypes, returnType));
    FunctionType::Signature *signature = funcDecl->getSignature();

    // check if the function is overloaded
    Symbol* overloadedFunc = symbolTable.lookupLocalSymbol(funcDecl->getName());
    if (overloadedFunc) {
        shared_ptr<FunctionType> existingFuncTypePtr = dyn_cast<FunctionType>(overloadedFunc->getType()).shared_from_this();
        if (!existingFuncTypePtr) {
            ErrorReporter::reportError("Symbol '" + funcDecl->getName() + "' is not a function");
            return;
        }
        existingFuncTypePtr->addOverload(signature);
        funcDecl->setFunctionType(existingFuncTypePtr);
    } else {
        // create a new function type and declare it
        shared_ptr<FunctionType> funcType = make_shared<FunctionType>(funcDecl->getName(), signature);
        if (!symbolTable.declare(std::move(make_unique<Symbol>(funcType.get())))) {
            ErrorReporter::reportError("Function '" + funcDecl->getName() + "' already declared");
            return;
        }
        funcDecl->setFunctionType(funcType);
    }

    // enter function scope
    shared_ptr<FunctionScope> funcScope = make_shared<FunctionScope>(symbolTable.currentScope(), funcDecl->getName(), signature);
    funcDecl->setScope(funcScope);
    symbolTable.enterScope(funcScope);

    // declare the function's parameters in the function scope
    for (const auto &paramSymbol : paramSymbols) {
        if (!funcScope->declare(std::move(paramSymbol))) {
            ErrorReporter::reportError("Parameter '" + param->getName() + "' already declared in function '" + funcDecl->getName() + "'");
            return;
        }
    }

    // collect the function's body statements
    collectTypeDeclarations(funcDecl->getBody());

    // exit function scope
    symbolTable.exitScope();
}


void lox::TypeInferenceEngine::inferStatements(const vector<unique_ptr<StmtBase>> &statements) {
    for (const auto &stmt : statements) {
        inferStatement(stmt.get());
    }
}

void lox::TypeInferenceEngine::inferStatement(StmtBase *stmt) {
    if (auto varDecl = dyn_cast<VarDeclStmt *>(stmt)) {
        inferVarDeclStmt(varDecl);
    } else if (auto funcDecl = dyn_cast<FunctionDeclStmt *>(stmt)) {
        inferFunctionDeclStmt(funcDecl);
    } else if (auto classDecl = dyn_cast<ClassDeclStmt *>(stmt)) {
        inferClassDeclStmt(classDecl);
    } else if (auto blockStmt = dyn_cast<BlockStmt *>(stmt)) {
        inferBlockStmt(blockStmt);
    } else {
        ErrorReporter::reportError("Unknown statement type in type inference engine");
    }
}

void lox::TypeInferenceEngine::inferVarDeclStmt(VarDeclStmt *varDecl) {
    const Type *varType;

    // check if varDecl has an initializer
    if (varDecl->getInitializer()) {
        const Type *initType = inferExpr(varDecl->getInitializer());

        optional<string> typeAnnotation = varDecl->getTypeAnnotation();
        if (typeAnnotation) {
            const Type *declaredType = symbolTable.lookupType(*typeAnnotation);
            if (!declaredType) {
                ErrorReporter::reportError("Type '" + *typeAnnotation + "' not found for variable '" + varDecl->getName() + "'");
                return;
            }

            // add a constraint between the initializer type and the declared type
            addConstraint(initType, declaredType, Constraint::ConstraintType::ASSIGNABLE);
            varType = declaredType;
        } else {
            // if no type is declared, we can infer the type from the initializer
            varType = initType;
        }
    }
    else {
        // if no initializer, we check if a type is declared
        optional<string> typeAnnotation = varDecl->getTypeAnnotation();
        if (typeAnnotation) {
            shared_ptr<Type> declaredType = symbolTable.lookupType(*typeAnnotation);
            if (!declaredType) {
                ErrorReporter::reportError("Type '" + *typeAnnotation + "' not found for variable '" + varDecl->getName() + "'");
                return;
            }

            varType = declaredType;
        }
        else {
            // if no initializer and no type declared, we use a type variable
            varType = TypeVariable::create();
        }
    }

    if (!symbolTable.declare(make_unique<Symbol>(varDecl->getName(), varType))) {
        ErrorReporter::reportError("Variable '" + varDecl->getName() + "' already declared");
        return;
    }
    varDecl->setType(varType);
}

void lox::TypeInferenceEngine::inferFunctionDeclStmt(FunctionDeclStmt *funcDecl) {
    // restore the function scope to the symbol table
    FunctionScope* funcScope = cast<FunctionScope>(funcDecl->getScope());
    if (!funcScope) {
        ErrorReporter::reportError("Function '" + funcDecl->getName() + "' has no scope");
        return;
    }
    symbolTable.enterScope(funcScope);

    inferStatements(funcDecl->getBody()->getStatements());
    funcDecl->getBody()->walk([&](ReturnStmt *returnStmt) {
        Type *returnType = nullptr;
        if (returnStmt->getValue()) {
            Type *returnType = returnStmt->getValue()->getType();
            if (!returnType) {
                ErrorReporter::reportError("Return statement has no type");
                return;
            }
            // add a constraint between the return type and the function's return type
            addConstraint(returnType, funcDecl->getSignature()->getReturnType(), Constraint::ConstraintType::EQUAL);
        }
        else {
            returnType = NilType::create();
            addConstraint(NilType::create(), returnStmt->getFunction()->getSignature()->getReturnType(), Constraint::ConstraintType::RETURN);
        }
        FunctionType::Signature *signature = funcDecl->getSignature();
        if (signature->getReturnType() == nullptr) {
            signature->setReturnType(returnType);
        } else {
            addConstraint(signature->getReturnType(), returnType, Constraint::ConstraintType::EQUAL);
        }
    });

    symbolTable.exitScope();
}

void lox::TypeInferenceEngine::inferClassDeclStmt(ClassDeclStmt *classDecl) {
    // restore the class scope to the symbol table
    ClassScope* classScope = cast<ClassScope>(classDecl->getScope());
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

void lox::TypeInferenceEngine::inferBlockStmt(BlockStmt *blockStmt) {
    // restore the block scope to the symbol table
    BlockScope* blockScope = cast<BlockScope>(blockStmt->getScope());
    symbolTable.enterScope(blockScope);

    // infer the block's statements
    inferStatements(blockStmt->getStatements());

    symbolTable.exitScope();
}

const Type * lox::TypeInferenceEngine::inferExpr(ExprBase *expr, const Type *expectedType) {
    switch (expr->getClassID())
    {
    case getClassIdOf<NumberExpr>():
        return lox::TypeInferenceEngine::NumberType.get();
    case getClassIdOf<StringExpr>():
        return lox::TypeInferenceEngine::StringType.get();
    case getClassIdOf<BoolExpr>():
        return lox::TypeInferenceEngine::BoolType.get();
    case getClassIdOf<NilExpr>():
        return lox::TypeInferenceEngine::NilType.get();
    case getClassIdOf<VariableExpr>():
    {
        VariableExpr *varExpr = cast<VariableExpr>(expr);
        Symbol *symbol = symbolTable.lookupLocalSymbol(varExpr->getName());
        if (!symbol) {
            ErrorReporter::reportError("Variable '" + varExpr->getName() + "' was not declared");
            return nullptr;
        }
        return symbol->getType();
    }
    case getClassIdOf<BinaryExpr>():
        return inferBinaryExpr(cast<BinaryExpr>(expr), expectedType);
    case getClassIdOf<UnaryExpr>():
        return inferUnaryExpr(cast<UnaryExpr>(expr), expectedType);
    case getClassIdOf<CallExpr>():
        return inferCallExpr(cast<CallExpr>(expr), expectedType);
    case getClassIdOf<AssignExpr>():
        return inferAssignExpr(cast<AssignExpr>(expr), expectedType);
    case getClassIdOf<AccessExpr>():
        return inferAccessExpr(cast<AccessExpr>(expr), expectedType);

    default:
        assert_not_reached("Unknown expression type in type inference engine");
    }
}

const Type *lox::TypeInferenceEngine::inferBinaryExpr(BinaryExpr *binaryExpr, const Type *expectedType) {
    const Type *leftType = inferExpr(binaryExpr->getLeft());
    const Type *rightType = inferExpr(binaryExpr->getRight());

    Type *resultType = nullptr;

    // Add constraints based on the operation
    switch (binaryExpr->getOp()) {
        case BinaryExpr::Op::Add:
        case BinaryExpr::Op::Subtract:
        case BinaryExpr::Op::Multiply:
        case BinaryExpr::Op::Divide:
            // self.add_constraint(left_type, right_type)
            addConstraint(leftType, rightType, Constraint::ConstraintType::EQUAL);
            if (expectedType) {
                addConstraint(leftType, expectedType, Constraint::ConstraintType::EQUAL);
            }
            resultType = leftType;
        case BinaryExpr::Op::Equal:
        case BinaryExpr::Op::NotEqual:
        case BinaryExpr::Op::LessThan:
        case BinaryExpr::Op::LessThanOrEqual:
        case BinaryExpr::Op::GreaterThan:
        case BinaryExpr::Op::GreaterThanOrEqual:
            addConstraint(leftType, rightType, Constraint::ConstraintType::EQUAL);
            resultType = lox::TypeInferenceEngine::BoolType.get();
        default:
            ErrorReporter::reportError("Unknown binary operation: " + BinaryExpr::toString(binaryExpr->getOp()));
            return nullptr;
    }

    binaryExpr->setType(resultType);
    return resultType;
}

const Type *lox::TypeInferenceEngine::inferUnaryExpr(UnaryExpr *unaryExpr, const Type *expectedType) {
    const Type *operandType = inferExpr(unaryExpr->getOperand());

    Type *resultType = nullptr;

    switch (unaryExpr->getOp()) {
        case UnaryExpr::Op::Negate:
            addConstraint(operandType, lox::TypeInferenceEngine::NumberType.get(), Constraint::ConstraintType::EQUAL);
            resultType = lox::TypeInferenceEngine::NumberType.get();
            break;
        case UnaryExpr::Op::Not:
            addConstraint(operandType, lox::TypeInferenceEngine::BoolType.get(), Constraint::ConstraintType::EQUAL);
            resultType = lox::TypeInferenceEngine::BoolType.get();
            break;
        default:
            ErrorReporter::reportError("Unknown unary operation: " + UnaryExpr::toString(unaryExpr->getOp()));
            return nullptr;
    }

    unaryExpr->setType(resultType);
    return resultType;
}

const Type *lox::TypeInferenceEngine::inferCallExpr(CallExpr *callExpr, const Type *expectedType) {
    Type *resultType = nullptr;

    vector<const Type*> argTypes;
    for (const auto &arg : callExpr->getArguments()) {
        const Type *argType = inferExpr(arg.get());
        argTypes.push_back(argType);
    }

    Type *calleeType = inferExpr(callExpr->getCallee().get());
    if (auto functionType = dyn_cast<FunctionType>(calleeType)) {
        // check if function has a matching signature
        FunctionType::Signature *bestMatch = functionType->resolveOverload(argTypes);
        if (!bestMatch) {
            ErrorReporter::reportError("No matching function overload found for call");
            return nullptr;
        }

        // add constraints for each argument
        for (size_t i = 0; i < argTypes.size(); ++i) {
            addConstraint(argTypes[i], bestMatch->getParameterType(i), Constraint::ConstraintType::ASSIGNABLE);
        }
        // set the return type of the call expression
        resultType = bestMatch->getReturnType();
        if (expectedType) {
            addConstraint(expectedType, resultType, Constraint::ConstraintType::ASSIGNABLE);
        }
    } else {
        ErrorReporter::reportError("Callee is not a function type");
        return nullptr;
    }

    callExpr->setType(resultType);
    return resultType;
}

const Type *lox::TypeInferenceEngine::inferAssignExpr(AssignExpr *assignExpr, const Type *expectedType) {
    const Type *targetType = inferExpr(assignExpr->getTarget());
    const Type *valueType = inferExpr(assignExpr->getValue());

    if (!targetType) {
        ErrorReporter::reportError("Target of assignment has no type");
        return nullptr;
    }

    // add a constraint between the target type and the value type
    addConstraint(valueType, targetType, Constraint::ConstraintType::ASSIGNABLE);
    assignExpr->setType(targetType);
    return targetType;
}

const Type *lox::TypeInferenceEngine::inferAccessExpr(AccessExpr *accessExpr, const Type *expectedType) {
    Type *objectType = inferExpr(accessExpr->getObject());
    if (isa<TypeVariable>(objectType)) {
        objectType = applySubstitutions(objectType);
    }

    if (auto classType = dyn_cast<ClassType>(objectType)) {
        // check if the field exists in the class
        const string &fieldName = accessExpr->getFieldName();
        const Type *fieldType = classType->getFieldType(fieldName);
        if (!fieldType) {
            ErrorReporter::reportError("Field '" + fieldName + "' not found in class '" + classType->getName() + "'");
            return nullptr;
        }
        accessExpr->setType(fieldType);
        return fieldType;
    } else {
        osstream ss;
        ss << "Unable to infer access expression '";
        accessExpr->print(ss);
        ErrorReporter::reportError(ss.str());
        return nullptr;
    }
}

void lox::TypeInferenceEngine::solveConstraints() {
    bool inferSuccess = true;
    for (const auto &constraint : constraints) {
        const Type *leftType = constraint.getLeft();
        const Type *rightType = constraint.getRight();
        Constraint::ConstraintType relation = constraint.getRelation();

        // Apply the constraint based on its type
        switch (relation) {
            case Constraint::ConstraintType::EQUAL:
                inferSuccess &= unify(leftType, rightType);
                break;
            case Constraint::ConstraintType::ASSIGNABLE:
                inferSuccess &= assinable(leftType, rightType);
                break;
        }
    }
    return inferSuccess;
}

bool lox::TypeInferenceEngine::unify(const Type *left, const Type *right) {
    Type *leftType = applySubstitutions(left);
    Type *rightType = applySubstitutions(right);

    if (leftType == rightType) {
        return true; // already unified
    }

    if (isa<TypeVariable>(leftType)) {
        if (occursCheck(cast<TypeVariable>(leftType), rightType)) {
            ErrorReporter::reportError("Type variable '" + leftType->getName() + "' occurs in '" + rightType->getName() + "'");
            return false;
        }
        substitutions[cast<TypeVariable>(leftType)] = rightType;
    }

    if (isa<TypeVariable>(rightType)) {
        if (occursCheck(cast<TypeVariable>(rightType), leftType)) {
            ErrorReporter::reportError("Type variable '" + rightType->getName() + "' occurs in '" + leftType->getName() + "'");
            return false;
        }
        substitutions[cast<TypeVariable>(rightType)] = leftType;
    }

    if (isa<FunctionType>(leftType) && isa<FunctionType>(rightType)) {
        assert_not_reached("Unimplemented FunctionType unification");
    }

    if (isa<ClassType>(leftType) && isa<ClassType>(rightType)) {
        const ClassType *leftClass = cast<ClassType>(leftType);
        const ClassType *rightClass = cast<ClassType>(rightType);

        if (leftClass != rightClass) {
            ErrorReporter::reportError("Cannot unify different class types: '" + leftClass->getName() + "' and '" + rightClass->getName() + "'");
            return false;
        }
        return true;
    }

    ErrorReporter::reportError("Cannot unify different types: '" + leftType->getName() + "' and '" + rightType->getName() + "'");
    return false;
}

bool lox::TypeInferenceEngine::assinable(const Type *left, const Type *right) {
    Type *from = applySubstitutions(left);
    Type *to = applySubstitutions(right);

    return from->isCompatibleWith(to);
}

bool lox::TypeInferenceEngine::occursCheck(const TypeVariable *var, const Type *type) {
    Type *subStitutedType = applySubstitutions(type);

    if (var == subStitutedType) {
        return true; // Type variable occurs in the type
    }

    if (auto classType = dyn_cast<ClassType>(subStitutedType)) {
        // Check if the type variable occurs in the class's fields
        for (const auto &field : classType->getFields()) {
            if (occursCheck(var, field.second)) {
                return true;
            }
        }
    } else if (auto functionType = dyn_cast<FunctionType>(subStitutedType)) {
        // Check if the type variable occurs in the function's parameters or return type
        for (const auto &param : functionType->getSignature()->getParameters()) {
            if (occursCheck(var, param)) {
                return true;
            }
        }
        if (occursCheck(var, functionType->getSignature()->getReturnType())) {
            return true;
        }
    }
}

void lox::TypeInferenceEngine::applySubstitutions(vector<unique_ptr<StmtBase>> &statements) {
    Walk astWalker();
    astWalker.registerCallback<ExprBase>([this](ExprBase *expr) {
        if (auto typeVar = dyn_cast<TypeVariable>(expr->getType())) {
            expr->setType(applySubstitution(typeVar));
        }
        return WalkResult::Advance;
    });

    astWalker.registerCallback<Declaration>([this](Declaration *decl) {
        if (auto typeVar = dyn_cast<TypeVariable>(decl->getType())) {
            decl->setType(applySubstitution(typeVar));
        }
        return WalkResult::Advance;
    });

    for (auto &stmt : statements) {
        astWalker.walk(stmt);
    }
}

const Type *lox::TypeInferenceEngine::applySubstitutions(const Type *type) {
    if (auto typeVar = dyn_cast<TypeVariable>(type)) {
        auto it = substitutions.find(typeVar);
        if (it != substitutions.end()) {
            return it->second;
        }
    }
    ErrorReporter::reportError("Type variable not found in substitutions: " + type->getName());
    assert_not_reached("Type variable not found in substitutions: " + type->getName());
    return type; // No substitution found, return the original type
}
