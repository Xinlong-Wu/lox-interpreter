#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "Compiler/Sema/Scope.h"
#include "Compiler/Sema/TypeSystem/TypeContext.h"

namespace lox {
class SymbolTable {
private:
  std::vector<std::shared_ptr<Scope>> scopes;
  std::shared_ptr<Scope> globalScope;

  // 禁止复制和赋值
  // SymbolTable(const SymbolTable&) = delete;
  // SymbolTable& operator=(const SymbolTable&) = delete;
  // SymbolTable(SymbolTable&&) = delete;
  // SymbolTable& operator=(SymbolTable&&) = delete;

public:
  SymbolTable(TypeContext *typeContext) {
    globalScope = std::make_shared<GlobalScope>(typeContext);
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

  bool declareType(const std::string &name, Type *type) {
    return scopes.back()->declareType(name, type);
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

  std::optional<SymbolOrType> lookupSymbolOrType(const std::string &name) {
    return scopes.back()->lookupSymbolOrType(name);
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