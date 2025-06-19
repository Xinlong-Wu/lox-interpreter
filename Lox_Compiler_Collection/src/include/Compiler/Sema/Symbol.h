#ifndef SYMBOL_H
#define SYMBOL_H

#include "Compiler/AST/Type.h"
#include "Compiler/ErrorReporter.h"

namespace lox {
class Symbol {
protected:
  std::string name;
  Type* type;
  bool _isDefined = false;
  bool _isUsed = false;
  bool isMutable = true;

public:
  Symbol(const std::string &name, Type* type = nullptr)
      : name(name), type(std::move(type)) {}
  Symbol(std::shared_ptr<FunctionType> funcType)
      : name(funcType->getName()), type(std::move(funcType)) {
    if (type == nullptr) {
      ErrorReporter::reportError("Function type cannot be null for symbol '" +
                                  name + "'.");
    }
    _isDefined = true; // Functions are defined when created
  }
  Symbol(std::shared_ptr<ClassType> classType)
      : name(classType->getName()), type(std::move(classType)) {
    if (type == nullptr) {
      ErrorReporter::reportError("Class type cannot be null for symbol '" +
                                  name + "'.");
    }
    _isDefined = true; // Classes are defined when created
  }

  const std::string &getName() const { return name; }

  bool hasType() const { return type != nullptr; }

  const Type* getType() const { return type; }

  void setType(Type* t) { type = t; }

  void markAsDefined() { _isDefined = true; }

  bool isDefined() const { return _isDefined; }

  void markAsUsed() {
    if (!_isDefined) {
      // // If the symbol is not defined, we should not mark it as used.
      // // This can happen if the symbol is used before it is defined.
      ErrorReporter::reportError("Symbol '" + name +
                                 "' is used before it is defined.");
      return;
    }
    _isUsed = true;
  }

  bool isUsed() const { return _isUsed; }

  virtual size_t hash() const {
    std::size_t seed = 0;
    lox::hash_combine(name, seed);
    if (type) {
      lox::hash_combine(type->hash(), seed);
    }
    return seed;
  }

  friend std::ostream &operator<<(std::ostream &os, const Symbol &sym) {
    sym.print(os);
    return os;
  }

  void print(std::ostream &os) const {
    os << name;
    if (type) {
      os << ": ";
      type->print(os);
    }
  }

  virtual void dump() const {
    this->print(std::cout);
    std::cout << std::endl;
  }
};

// Hash function for Symbol
struct SymbolHash {
  std::size_t operator()(const Symbol &sym) const {
    std::size_t seed = 0;
    lox::hash_combine(sym.getName(), seed);
    if (sym.hasType()) {
      lox::hash_combine(sym.getType()->hash(), seed);
    }
    return seed;
  }
};
} // namespace lox

#endif // SYMBOL_H