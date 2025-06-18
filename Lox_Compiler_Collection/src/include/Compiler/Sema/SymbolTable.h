#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "Compiler/Sema/Scope.h"

namespace lox {
class SymbolTable {
private:
  std::vector<std::shared_ptr<Scope>> scopes;

public:
  SymbolTable() { scopes.push_back(std::make_shared<GlobalScope>()); }
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

  bool declare(std::shared_ptr<Symbol> sym) {
    return scopes.back()->declare(sym);
  }

  bool declareType(const std::string &name, std::shared_ptr<Type> type) {
    return scopes.back()->declareType(name, type);
  }

  std::shared_ptr<Symbol> lookupSymbol(const std::string &name) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto sym = (*it)->resolve(name);
      if (sym) {
        return sym;
      }
    }
    return nullptr;
  }

  std::shared_ptr<Symbol> lookupLocalSymbol(const std::string &name) {
    return scopes.back()->resolveLocal(name);
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