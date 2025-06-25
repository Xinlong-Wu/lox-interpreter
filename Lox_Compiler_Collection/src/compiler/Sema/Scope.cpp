#include "Compiler/Sema/TypeSystem/TypeContext.h"
#include "Compiler/Sema/Scope.h"

using namespace std;

size_t lox::BlockScope::anonymousCounter = 0;

lox::GlobalScope::GlobalScope(TypeContext *typeContext) : ScopeBase(nullptr, "Global") {
    // declear built-in functions and types
    declareType("Number", typeContext->getNumberType());
    declareType("String", std::move(typeContext->getStringType()));
    declareType("Bool", std::move(typeContext->getBoolType()));
    declareType("Nil", std::move(typeContext->getNilType()));

    // declare built-in functions
    FunctionType *printFunc = typeContext->make<FunctionType>("print");
    printFunc->addOverload({typeContext->getStringType()}, typeContext->getNilType());
    declare(std::make_unique<Symbol>("print", printFunc));
}