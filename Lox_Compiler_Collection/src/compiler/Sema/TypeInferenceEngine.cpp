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
    vector<shared_ptr<Type>> paramTypes;
    for(const auto &param : funcDecl->getParameters()) {
        if (param->getTypeName()) {
            auto type = symbolTable.lookupType(*param->getTypeName());
            if (!type) {
                ErrorReporter::reportError("Type '" + *param->getTypeName() + "' not found for parameter '" + param->getName() + "'");
                return;
            }
            paramTypes.push_back(type);
        } else {
            paramTypes.push_back(make_shared<TypeVariable>());
        }
    }

    // collect the function's return type
    shared_ptr<Type> returnType = make_shared<TypeVariable>();

    lox::FunctionType::Signature signature(paramTypes, returnType);

    // check if the function is overloaded
    shared_ptr<Symbol> overloadedFunc = symbolTable.lookupLocalSymbol(funcDecl->getName());
    if (overloadedFunc) {
        shared_ptr<FunctionType> existingFuncType = dyn_cast<FunctionType>(overloadedFunc->getType());
        if (!existingFuncType) {
            ErrorReporter::reportError("Symbol '" + funcDecl->getName() + "' is not a function");
            return;
        }

        existingFuncType->addOverload(signature);
    } else {
        // create a new function type and declare it
        shared_ptr<FunctionType> funcType = make_shared<FunctionType>(funcDecl->getName(), signature);
        shared_ptr<Symbol> funcSymbol = make_shared<Symbol>(funcType);
        if (!symbolTable.declare(funcSymbol)) {
            ErrorReporter::reportError("Function '" + funcDecl->getName() + "' already declared");
            return;
        }
    }

    // enter function scope
    shared_ptr<FunctionScope> funcScope = make_shared<FunctionScope>(symbolTable.currentScope(), funcDecl->getName());
    symbolTable.enterScope(funcScope);
    // collect the function's body statements

    collectTypeDeclarations(funcDecl->getBody()->getStatements());

    // exit function scope
    symbolTable.exitScope();
}



