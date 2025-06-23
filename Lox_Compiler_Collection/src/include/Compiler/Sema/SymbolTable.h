#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "Compiler/Sema/Scope.h"

namespace lox {
class SymbolTable {
private:
  std::vector<std::shared_ptr<Scope>> scopes;
  std::shared_ptr<Scope> globalScope;

public:
  SymbolTable() {
    globalScope = std::make_shared<GlobalScope>();
    scopes.push_back(globalScope);
  }
  ~SymbolTable() = default;

  void enterScope(const std::shared_ptr<Scope> &scope) {
    scopes.push_back(scope);
  }

  void exitScope() {
    if (scopes.size() > 1) {
      scopes.pop_back();
    } else {
      ErrorReporter::reportError("Cannot exit the global scope");
    }
  }

  std::shared_ptr<Scope> currentScope() const { return scopes.back(); }

  bool declare(std::unique_ptr<Symbol> sym) {
    return scopes.back()->declare(std::move(sym));
  }

  bool declareType(const std::string &name, std::unique_ptr<Type> type) {
    return scopes.back()->declareType(name, std::move(type));
  }

  Symbol* lookupSymbol(const std::string &name) {
    return scopes.back()->lookup(name);
  }

  Symbol* lookupLocalSymbol(const std::string &name) {
    return scopes.back()->lookupLocal(name);
  }

  Type* lookupType(const std::string &name) {
    return scopes.back()->lookupType(name);
  }

  Type* lookupTypeLocal(const std::string &name) {
    return scopes.back()->lookupTypeLocal(name);
  }

  void print(std::ostream &os) const {
    for (size_t i = 0; i < scopes.size(); i++) {
      scopes[i]->print(os, i);
    }
  }
  virtual void dump() const {
    this->print(std::cout);
    std::cout << std::endl;
  }
};
} // namespace lox

#endif // SYMBOLTABLE_H