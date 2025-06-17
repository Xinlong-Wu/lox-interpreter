#ifndef SCOPE_H
#define SCOPE_H

#include "Common.h"
#include "Compiler/Sema/Symbol.h"

namespace lox {
class FunctionScope;
class ClassScope;

class Scope {
protected:
  using ClassID = const void*;

  std::string name;
  std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
  std::unordered_map<std::string, std::shared_ptr<Type>> types;
  std::shared_ptr<Scope> enclosingScope;
public:
  Scope(std::shared_ptr<Scope> parent, const std::string &name)
      : enclosingScope(parent), name(name) {}
  virtual ~Scope() = default;

  // 纯虚接口
  const std::string& getName() const { return name; }
  std::shared_ptr<Scope> getEnclosingScope() const { return enclosingScope; }

  // 作用域检查
  virtual bool inFunctionScope() const = 0;
  virtual bool inClassScope() const = 0;

  // 类型获取
  virtual std::shared_ptr<FunctionType> getCurrentFunctionType() const = 0;
  virtual std::shared_ptr<ClassType> getCurrentClassType() const = 0;

  // 符号管理
  bool declare(std::shared_ptr<Symbol> &symbol) {
    if (lookupLocal(symbol->getName())) {
        ErrorReporter::reportError("Symbol '" + symbol->getName() +
                                  "' is already declared in scope '" +
                                  this->getName() + "'");
        return false;
    }
    if (lookupTypeLocal(symbol->getName())) {
        ErrorReporter::reportError("Symbol '" + symbol->getName() +
                                  "' is conflicting with a type in scope '" +
                                  this->getName() + "'");
        return false;
    }
    symbols[symbol->getName()] = symbol;
    return true;
  }

  bool declareType(const std::string &name, std::shared_ptr<Type> type) {
    if (lookupLocal(name)) {
        ErrorReporter::reportError("Type '" + name +
                                  "' is conflicting with a symbol in scope '" +
                                  this->getName() + "'");
        return false;
    }
    if (lookupTypeLocal(name)) {
        ErrorReporter::reportError("Type '" + name +
                                  "' is already declared in scope '" +
                                  this->getName() + "'");
        return false;
    }
    types[name] = std::move(type);
    return true;
  }

  std::shared_ptr<Symbol> lookup(const std::string &name) {
      auto it = symbols.find(name);
      if (it != symbols.end()) {
          return it->second;
      }
      return enclosingScope ? enclosingScope->lookup(name) : nullptr;
  }

  std::shared_ptr<Type> lookupType(const std::string &name) {
      auto it = types.find(name);
      if (it != types.end()) {
          return it->second;
      }
      return enclosingScope ? enclosingScope->lookupType(name) : nullptr;
  }

  std::shared_ptr<Symbol> lookupLocal(const std::string &name) {
      auto it = symbols.find(name);
      return (it != symbols.end()) ? it->second : nullptr;
  }

  std::shared_ptr<Type> lookupTypeLocal(const std::string &name) {
    auto it = types.find(name);
    return (it != types.end()) ? it->second : nullptr;
  }

  // 工具函数
  // virtual void checkUnusedSymbols() const = 0;
  size_t hash() const {
    size_t seed = 0;
    lox::hash_combine(name, seed);
    for (const auto &[name, symbol] : symbols) {
      lox::hash_combine(symbol->hash(), seed);
    }
    for (const auto &[name, type] : types) {
      lox::hash_combine(type->hash(), seed);
    }
    return seed;
  }

  virtual ClassID getClassID() const = 0;
  virtual void print(std::ostream &os, int level = 0) const = 0;

  void dump(int level = 0) const {
    print(std::cout, level);
    std::cout << std::endl;
  };
};


// CRTP基类模板，同时实现接口
template<typename Derived>
class ScopeBase : public Scope {
protected:
  mutable std::optional<bool> _inClassScope = std::nullopt;
  mutable std::optional<bool> _inFunctionScope = std::nullopt;
  mutable std::shared_ptr<ClassType> currentClassType = nullptr;
  mutable std::shared_ptr<FunctionType> currentFunctionType = nullptr;

protected:
  // 存储实际的类型ID
  ClassID classID;
  // 获取类型的唯一ID
  template<typename T>
  static ClassID _getClassID() {
    static char id;
    return &id;
  }

  // CRTP辅助函数
  Derived& derived() { return static_cast<Derived&>(*this); }
  const Derived& derived() const { return static_cast<const Derived&>(*this); }

public:
  ScopeBase(std::shared_ptr<Scope> parent, const std::string &name)
        : Scope(parent, name) {}

  virtual ~ScopeBase() = default;

  // 实现复杂的缓存逻辑
  bool inFunctionScope() const override {
      if (_inFunctionScope.has_value()) {
          return _inFunctionScope.value();
      }

      const Scope* current = this;
      while (current != nullptr) {
          if (isa<FunctionScope>(current)) {
              _inFunctionScope = true;
              return true;
          }
          current = current->getEnclosingScope().get();
      }
      _inFunctionScope = false;
      return false;
  }

  bool inClassScope() const override {
      if (_inClassScope.has_value()) {
          return _inClassScope.value();
      }

      const Scope* current = this;
      while (current != nullptr) {
          if (isa<ClassScope>(current)) {
              _inClassScope = true;
              return true;
          }
          current = current->getEnclosingScope().get();
      }
      _inClassScope = false;
      return false;
  }

  std::shared_ptr<FunctionType> getCurrentFunctionType() const override {
    if (!inFunctionScope()) {
        return nullptr;
    }

    if (currentFunctionType != nullptr) {
        return currentFunctionType;
    }

    // 让派生类提供具体实现
    if (auto funcType = derived().getCurrentFunctionTypeImpl()) {
        currentFunctionType = funcType;
        return funcType;
    }

    // 向外层作用域查找
    if (enclosingScope) {
        auto funcType = enclosingScope->getCurrentFunctionType();
        if (funcType) {
            currentFunctionType = funcType;
        }
        return funcType;
    }
    return nullptr;
  }

  std::shared_ptr<ClassType> getCurrentClassType() const override {
    if (!inClassScope()) {
        return nullptr;
    }

    if (currentClassType != nullptr) {
        return currentClassType;
    }

    // 让派生类提供具体实现
    if (auto classType = derived().getCurrentClassTypeImpl()) {
        currentClassType = classType;
        return classType;
    }

    // 向外层作用域查找
    if (enclosingScope) {
        auto classType = enclosingScope->getCurrentClassType();
        if (classType) {
            currentClassType = classType;
        }
        return classType;
    }
    return nullptr;
  }


  ClassID getClassID() const override { return classID; }

  static bool classof(const Scope* expr) {
    return expr->getClassID() == _getClassID<Derived>();
  }

  // 默认实现，派生类可以重写
  virtual std::shared_ptr<FunctionType> getCurrentFunctionTypeImpl() const { return nullptr; }
  virtual std::shared_ptr<ClassType> getCurrentClassTypeImpl() const { return nullptr; }
};

// 具体的作用域类型实现
class GlobalScope : public ScopeBase<GlobalScope> {
public:
  GlobalScope() : ScopeBase(nullptr, "Global") {}
};

class ClassScope : public ScopeBase<ClassScope> {
public:
  ClassScope(std::shared_ptr<Scope> parent, const std::string &name)
        : ScopeBase(parent, name) {
    if (!parent) {
      ErrorReporter::reportError("Class scope must have an enclosing scope");
      return;
    }

    this->currentClassType = cast<ClassType>(parent->lookupType(name));
    if (this->currentClassType) {
      ErrorReporter::reportError("Class '" + name + "' is not defined in enclosing scope");
    }
  }

  std::shared_ptr<ClassType> getCurrentClassTypeImpl() const override {
    return currentClassType;
  }
};

class FunctionScope : public ScopeBase<FunctionScope> {
public:
  FunctionScope(std::shared_ptr<Scope> parent, const std::string &name)
      : ScopeBase(parent, name) {
    this->currentFunctionType = cast<FunctionType>(parent->lookupType(name));
    if (this->currentFunctionType == nullptr) {
      ErrorReporter::reportError("Function '" + name + "' is not defined in enclosing scope");
    }
  }

  std::shared_ptr<FunctionType> getCurrentFunctionTypeImpl() const override {
    return currentFunctionType;
  }
};

class BlockScope : public ScopeBase<BlockScope> {
public:
  BlockScope(std::shared_ptr<ScopeBase> parent, const std::string &name)
      : ScopeBase(parent, name) {}
};
}
#endif // SCOPE_H