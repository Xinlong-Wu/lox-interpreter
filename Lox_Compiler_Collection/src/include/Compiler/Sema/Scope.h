#ifndef SCOPE_H
#define SCOPE_H

#include "Common.h"
#include "Compiler/Sema/Symbol.h"

namespace lox {
class FunctionScope;
class ClassScope;
class Scope {
protected:
  enum class Kind { GlobalScope, ClassScope, FunctionScope, BlockScope };

  std::string name;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  const std::shared_ptr<Scope> enclosingScope; // 外层作用域
public:
  Scope(std::shared_ptr<Scope> parent, const std::string &name,
        bool isClassScope = false, bool isFunctionScope = false)
      : enclosingScope(parent), name(name) {}
  virtual ~Scope() = default;

  virtual bool inFunctionScope() const { 
    const Scope *current = this;
    while (current != nullptr) {
      if (isa<FunctionScope>(current)) {
        return true; // 如果当前作用域是函数作用域，返回true
      }
      current = current->enclosingScope.get(); // 向外层作用域移动
    }
    return false; // 如果没有找到函数作用域，返回false
  }
  virtual bool inClassScope() const {
    const Scope *current = this;
    while (current != nullptr) {
      if (isa<ClassScope>(current)) {
        return true; // 如果当前作用域是类作用域，返回true
      }
      current = current->enclosingScope.get(); // 向外层作用域移动
    }
    return false; // 如果没有找到类作用域，返回false
  }

  virtual std::shared_ptr<Symbol> getCurrentClassSymbol() const {
    if (enclosingScope == nullptr) {
      return nullptr;
    }
    return enclosingScope->getCurrentClassSymbol();
  }

  virtual bool setCurrentReturnType(std::shared_ptr<Type> type) {
    if (enclosingScope == nullptr) {
      return false; // 如果没有外层作用域，无法设置返回类型
    }
    return enclosingScope->setCurrentReturnType(std::move(type));
  }
  virtual const std::vector<std::shared_ptr<Type>>* getCurrentReturnTypes() const {
    if (enclosingScope == nullptr) {
      return nullptr;
    }
    return enclosingScope->getCurrentReturnTypes();
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
private:
std::shared_ptr<Symbol> currentClassSymbol = nullptr; // 当前类符号
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

  virtual std::shared_ptr<Symbol> getCurrentClassSymbol() const override {
    return currentClassSymbol;
  }

  TYPEID_SYSTEM(Scope, ClassScope)
};

class FunctionScope : public Scope {
private:
  std::shared_ptr<std::vector<std::shared_ptr<Type>>> allReturnTypes = std::make_shared<std::vector<std::shared_ptr<Type>>>(); // 所有返回类型
public:
  FunctionScope(const std::shared_ptr<Scope> &parent, const std::string &name)
      : Scope(parent, name, false, true) {}

  virtual bool setCurrentReturnType(std::shared_ptr<Type> type) override {
    allReturnTypes->push_back(std::move(type));
    return true;
  }
  virtual const std::vector<std::shared_ptr<Type>>* getCurrentReturnTypes() const override {
    return allReturnTypes.get();
  }

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