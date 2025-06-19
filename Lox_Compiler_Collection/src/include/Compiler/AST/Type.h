#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <memory>
#include <string>

#include "Common.h"
#include "Compiler/ErrorReporter.h"

namespace lox {
class Scope;
class ClassScope;

class Type {
protected:
  std::string name;
public:
  Type(const std::string &name) : name(name) {}
  virtual ~Type() = default;

  std::string getName() const { return name; }

  virtual bool isCompatibleWith(Type* other) {
    return this == other;
  }

  virtual bool operator==(const Type &other) const{
    return &other == this;
  }
  virtual bool operator!=(const Type &other) const { return !(*this == other); }

  virtual void print(std::ostream &os) const = 0;
  virtual void dump() const {
    print(std::cout);
    std::cout << std::endl;
  }

  virtual size_t hash() const {
    size_t seed = 0;
    hash_combine(name, seed);
    return seed;
  }

  virtual ClassID getTypeID() const = 0;
};

template<typename Derived>
class TypeBase : public Type {
public:
  TypeBase(const std::string &name)
      : Type(name) {};

  ClassID getTypeID() const override {
    return getClassIdOf<Derived>();
  }

  static bool classof(const Type *type) {
    return type->getTypeID() == getClassIdOf<Derived>();
  }

  void print(std::ostream &os) const override {
    cast<Derived>(this)->printImpl(os);
  }

  virtual void printImpl(std::ostream &os) const = 0;
};

class TypeVariable : public TypeBase<TypeVariable> {
private:
  inline static size_t id = 0;
public:
  TypeVariable() : TypeBase("T" + std::to_string(id++)) {}
  ~TypeVariable() override = default;

  void printImpl(std::ostream &os) const override {
    os << name;
  }
};

class PrimitiveType : public TypeBase<PrimitiveType> {
public:
  PrimitiveType(std::string name) : TypeBase(std::move(name)) {}
  ~PrimitiveType() override = default;

  void printImpl(std::ostream &os) const override {
    os << name;
  }
};

class NilType : public TypeBase<NilType> {
private:
  // Singleton instance for NilType
  static std::shared_ptr<NilType> instance;
  NilType() : TypeBase("nil") {}
public:
  ~NilType() override = default;

  static std::shared_ptr<NilType> create() {
    if (!instance) {
      instance = std::shared_ptr<NilType>(new NilType());
    }
    return instance;
  }

  // bool isCompatibleWith(Type* other) override {
  //   return other->getTypeID() == getClassIdOf<NilType>();
  // }

  void printImpl(std::ostream &os) const override {
    os << "nil";
  }
};

class ClassType : public TypeBase<ClassType> {
private:
  const ClassType* superclass = nullptr;
  ClassScope* properties = nullptr;
public:
  ClassType(const std::string &name, const ClassType* superClass)
    : TypeBase(name), superclass(superClass) {}
  ClassType(const std::string &name)
    : TypeBase(name) {}

  ~ClassType() override = default;
  std::string getName() const { return name; }

  bool isCompatibleWith(Type* other) override {
    if (this == other) {
      return true;
    }
    if (auto classType = dyn_cast<ClassType*>(other)) {
      // Check if this class is a subclass of the other class
      ClassType* current = this->getSuperClass();
      while (current) {
        if (current == classType) {
          return true;
        }
        current = current->superclass;
      }
    }
    return false;
  }

  const ClassType *getSuperClass() const {
    return superclass.get();
  }

  void printImpl(std::ostream &os) const override {
    os << "class " << name;
  }
};

class FunctionType : public std::enable_shared_from_this<MyClass>,
                      public TypeBase<FunctionType> {
public:
  struct Signature {
    std::vector<Type*> parameters;
    Type* returnType;

    Signature(const std::vector<Type*> parameters,
              Type* returnType = nullptr)
        : parameters(std::move(parameters)), returnType(returnType) {}

    Signature(const Signature &other)
        : parameters(other.parameters), returnType(other.returnType) {}

    Type *getParameterType(size_t index) const {
      assert(index < parameters.size() && "Index out of bounds");
      if (index < parameters.size()) {
        return parameters[index];
      }
      return nullptr;
    }

    FunctionType::Signature *resolveOverload(const std::vector<Type*> &argTypes) const;

    bool operator==(const Signature &other) const {
      if (this == &other) {
        return true;
      }
      if (parameters.size() != other.parameters.size()) {
        return false;
      }
      for (size_t i = 0; i < parameters.size(); ++i) {
        if (*parameters[i] != *other.parameters[i]) {
          return false;
        }
      }
      return returnType == other.returnType;
    }
    bool operator!=(const Signature &other) const { return !(*this == other); }

    size_t hash() const {
      size_t seed = 0;
      for (const auto &param : parameters) {
        hash_combine(param->hash(), seed);
      }
      if (returnType) {
        hash_combine(returnType->hash(), seed);
      }
      return seed;
    }
  };
private:
  std::vector<Signature*> overloads;
public:
  FunctionType(std::string name) : TypeBase(name) {}
  FunctionType(std::string name, const Signature *signature)
      : TypeBase(name) {
    overloads.push_back(signature);
  }

  ~FunctionType() override = default;

  bool isCompatibleWith(Type* other) override {
    assert(false && "Unimplemented FunctionType isCompatibleWith");
    return false;
  }

  std::string getName() const { return name; }

  bool hasOverload(const Signature *signature) const {
    return std::any_of(overloads.begin(), overloads.end(),
                       [&signature](const Signature *s) {
                        return *s == *signature;
                       });
  }

  void addOverload(const Signature *signature) {
    if (hasOverload(signature)) {
      ErrorReporter::reportError(
          "Function '" + name + "' already has an overload with the same "
                             "signature.");
      return;
    }
    overloads.push_back(signature);
  }

  bool operator==(const FunctionType *other) const {
    if (other == this) {
      return true;
    }
    if (name != other->name || overloads.size() != other->overloads.size()) {
      return false;
    }
    return std::equal(overloads.begin(), overloads.end(), other->overloads.begin(),
                      [](const std::shared_ptr<Signature> &a,
                         const std::shared_ptr<Signature> &b) { return *a == *b; });
  }

  size_t hash() const override {
    size_t seed = Type::hash();
    hash_combine(name, seed);
    for (const auto &overload : overloads) {
      hash_combine(overload->hash(), seed);
    }
    return seed;
  }

  void print(std::ostream &os) const override {
    os << "Function " << name << " with ";
    if (overloads.empty()) {
      os << "no overloads";
      return;
    }
    os << overloads.size() << " overloads ";
  }
};
} // namespace lox

#endif // TYPE_H