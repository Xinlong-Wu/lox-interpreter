#ifndef SCOPE_H
#define SCOPE_H

#include "Common.h"
#include "Compiler/Sema/Symbol.h"

namespace lox {
class FunctionScope;
class Scope {
protected:
  enum class Kind { GlobalScope, ClassScope, FunctionScope, BlockScope };

  std::string name;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  const std::shared_ptr<Scope> enclosingScope; // 外层作用域
  bool inFunctionScope = false;                // 是否在函数作用域内
  bool inClassScope = false;                   // 是否在类作用域内
  std::shared_ptr<Symbol> currentClassSymbol = nullptr; // 当前类符号
  std::shared_ptr<Type> currentReturnType = nullptr; // 当前函数返回类型
public:
  Scope(std::shared_ptr<Scope> parent, const std::string &name,
        bool inClassScope = false, bool inFunctionScope = false)
      : enclosingScope(parent), name(name) {
    assert(!inFunctionScope ||
           enclosingScope != nullptr &&
               "Function scope must have an enclosing scope");
    assert(!inClassScope || enclosingScope != nullptr &&
                                "Class scope must have an enclosing scope");
    if (enclosingScope) {
      this->inFunctionScope =
          enclosingScope->inFunctionScope || inFunctionScope;
      this->inClassScope = enclosingScope->inClassScope || inClassScope;
    } else {
      // 如果没有外层作用域，则直接设置
      this->inClassScope = inClassScope;
      this->inFunctionScope = inFunctionScope;
    }
    if (this->inClassScope) {
      // 如果在类作用域内，设置当前类符号
      currentClassSymbol =
          enclosingScope ? enclosingScope->currentClassSymbol : nullptr;
    }
  }
  virtual ~Scope() = default;

  virtual bool isInFunctionScope() const { return inFunctionScope; }
  virtual bool isInClassScope() const { return inClassScope; }
  virtual std::shared_ptr<Symbol> getCurrentClassSymbol() const {
    return currentClassSymbol;
  }

  virtual void setCurrentReturnType(std::shared_ptr<Type> type) {
    assert(isInFunctionScope() && "Current scope is not a function scope");
    currentReturnType = std::move(type);
  }
  virtual std::shared_ptr<Type> getCurrentReturnType() const {
    assert(isInFunctionScope() && "Current scope is not a function scope");
    return currentReturnType;
  }

  virtual bool declare(std::shared_ptr<Symbol> &symbol) {
    if (resolveLocal(symbol->getName())) {
      ErrorReporter::reportError("Symbol '" + symbol->getName() +
                                 "' is already declared in scope '" +
                                 this->getName() + "'");
      return false; // 如果符号已存在，返回false
    }
    symbols[symbol->getName()] = symbol;
    return true;
  }

  std::shared_ptr<Symbol> resolve(const std::string &name) {
    auto it = symbols.find(name);
    if (it != symbols.end()) {
      return it->second;
    }

    // 如果当前作用域没有找到，尝试外层作用域
    return enclosingScope ? enclosingScope->resolve(name) : nullptr;
  }

  std::shared_ptr<Symbol> resolveLocal(const std::string &name) {
    // 只在当前作用域查找，不查找外层作用域
    auto it = symbols.find(name);
    if (it != symbols.end()) {
      return it->second;
    }
    return nullptr;
  }

  std::shared_ptr<Scope> getEnclosingScope() const { return enclosingScope; }

  // 在scope退出时，检查是否有未定义和未使用的符号
  void checkUnusedSymbols() const {
    for (const auto &[name, symbol] : symbols) {
      if (!symbol->isDefinedSymbol()) {
        ErrorReporter::reportError("Symbol '" + name +
                                   "' is declared but not defined in scope '" +
                                   this->getName() + "'");
      } else if (!symbol->isUsedSymbol()) {
        ErrorReporter::reportWarning("Symbol '" + name +
                                     "' is defined but not used in scope '" +
                                     this->getName() + "'");
      }
    }
  }

  const std::string &getName() const { return name; }

  void print(std::ostream &os, int level = 0) const {
    std::string indent(level * 1, '\t');
    os << indent << "Scope: " << name << std::endl;

    for (const auto &[name, symbol] : symbols) {
      os << indent << " ";
      symbol->print(os);
      os << std::endl;
    }
  }

  virtual void dump(int level = 0) const {
    print(std::cout, level);
    std::cout << std::endl;
  }

  static bool classof(const Scope *ptr) {
    return ptr->getKind() == Kind::BlockScope;
  }
  static bool classof(const std::shared_ptr<Scope> ptr) {
    return Scope::classof(ptr.get());
  }
  virtual Kind getKind() const { return Kind::BlockScope; }
};

class GlobalScope : public Scope {
public:
  GlobalScope() : Scope(nullptr, "Global") {}

  TYPEID_SYSTEM(Scope, GlobalScope)
};

class ClassScope : public Scope {
public:
  ClassScope(std::shared_ptr<Scope> &parent, const std::string &name)
      : Scope(parent, name, true, false) {
    assert(parent != nullptr && "Class scope must have an enclosing scope");
    std::shared_ptr<Symbol> classSymbol = parent->resolveLocal(name);
    if (classSymbol == nullptr || !isa<ClassType>(classSymbol->getType())) {
      ErrorReporter::reportError("Class '" + name +
                                 "' is not defined in enclosing scope");
    } else {
      currentClassSymbol = classSymbol;
    }
  }

  TYPEID_SYSTEM(Scope, ClassScope)
};

class FunctionScope : public Scope {
public:
  FunctionScope(const std::shared_ptr<Scope> &parent, const std::string &name)
      : Scope(parent, name, false, true) {}

  TYPEID_SYSTEM(Scope, FunctionScope)
};

// class BlockScope : public Scope {
// public:
//     BlockScope(std::shared_ptr<Scope>& parent, const std::string& name)
//         : Scope(parent, name) {}

//     TYPEID_SYSTEM(Scope, BlockScope)
// };
} // namespace lox

#endif // SCOPE_H