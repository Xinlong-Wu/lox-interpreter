#include "Compiler/Sema/Scope.h"
#include "Compiler/Sema/TypeSystem/TypeContext.h"

using namespace std;

size_t lox::BlockScope::anonymousCounter = 0;

static void declareBuiltInTypes(lox::GlobalScope *globalScope,
                                lox::TypeContext *typeContext) {
  globalScope->declareType("Number", typeContext->getNumberType());
  globalScope->declareType("String", typeContext->getStringType());
  globalScope->declareType("Bool", typeContext->getBoolType());
  globalScope->declareType("Nil", typeContext->getNilType());
}

static void registPrintFunction(lox::GlobalScope *globalScope,
                                lox::TypeContext *typeContext) {
  lox::FunctionType *printFunc = typeContext->make<lox::FunctionType>("print");
  globalScope->declare(std::make_unique<lox::Symbol>("print", printFunc));

  // print ( String ) -> Nil
  printFunc->addOverload({typeContext->getStringType()},
                         typeContext->getNilType());

  // print ( Number ) -> Nil
  printFunc->addOverload({typeContext->getNumberType()},
                         typeContext->getNilType());

  // print ( Bool ) -> Nil
  printFunc->addOverload({typeContext->getBoolType()},
                         typeContext->getNilType());
}

lox::GlobalScope::GlobalScope(TypeContext *typeContext)
    : ScopeBase(nullptr, "Global") {
  // declear built-in functions and types
  declareBuiltInTypes(this, typeContext);

  // declare built-in functions
  registPrintFunction(this, typeContext);
}
