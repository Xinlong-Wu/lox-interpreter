#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <memory>
#include <string>

#include "Common.h"
#include "Compiler/ErrorReporter.h"

namespace lox {
class TypeContext;
class Scope;
class ClassScope;

class Type {
protected:
  std::string name;

  Type(const std::string &name) : name(name) {}
public:
  virtual ~Type() = default;
  std::string getName() const { return name; }

  virtual bool isCompatibleWith(const Type* other) const {
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
protected:
    TypeBase(const std::string &name)
        : Type(name) {};
    TypeBase& operator=(const TypeBase&) = delete;

    TypeBase(TypeBase&&) = default;
    TypeBase& operator=(TypeBase&&) = default;
public:
    ClassID getTypeID() const override {
      return ClassID::get<Derived>();
    }

  static bool classof(const Type *type) {
    return type->getTypeID() == ClassID::get<Derived>();
  }

  void print(std::ostream &os) const override {
    cast<Derived>(this)->printImpl(os);
  }

  virtual void printImpl(std::ostream &os) const = 0;
};

class TypeVariable : public TypeBase<TypeVariable> {
protected:
  static std::vector<std::unique_ptr<TypeVariable>> instances;

  TypeVariable(const std::string& name) : TypeBase(name) {}
public:
  ~TypeVariable() override = default;

  static TypeVariable* create(std::string name = "") {
    if (name.empty()) {
      name = "T" + std::to_string(instances.size());
    }

    auto newVar = std::unique_ptr<TypeVariable>(new TypeVariable(name));
    TypeVariable* ptr = newVar.get();
    instances.push_back(std::move(newVar));
    return ptr;
  }

  bool isCompatibleWith(const Type* other) const override {
    // Type variables are compatible with any type
    return true;
  }

  void printImpl(std::ostream &os) const override {
    os << name;
  }

  friend class TypeContext;
};

class PrimitiveType : public TypeBase<PrimitiveType> {
protected:
  PrimitiveType(std::string name) : TypeBase(std::move(name)) {}
public:
  ~PrimitiveType() override = default;
  void printImpl(std::ostream &os) const override {
    os << name;
  }

  friend class TypeContext;
};

class NilType : public TypeBase<NilType> {
private:
  // Singleton instance for NilType
  // static std::unique_ptr<NilType> instance;
protected:
  NilType() : TypeBase("nil") {}
public:
  ~NilType() override = default;
  void printImpl(std::ostream &os) const override {
    os << "nil";
  }

  friend class TypeContext;
};

class ClassType : public TypeBase<ClassType> {
private:
  ClassType* superclass = nullptr;
  ClassScope* properties = nullptr;
protected:
  ClassType(const std::string &name, ClassType* superClass)
    : TypeBase(name), superclass(superClass) {}
  ClassType(const std::string &name)
    : TypeBase(name) {}

public:
  ~ClassType() override = default;
  std::string getName() const { return name; }

  Type* getPropertyType(const std::string &propertyName) const;

  const std::vector<Type*> getPropertyTypes() const;

  const ClassScope* getClassScope() const {
    return cast<ClassScope>(properties);
  }

  void setClassScope(ClassScope* scope) {
    assert(properties == nullptr && "Class scope has already been set");
    properties = scope;
  }

  bool isCompatibleWith(const Type* other) const override {
    if (this == other) {
      return true;
    }
    if (auto classType = dyn_cast<const ClassType>(other)) {
      // Check if this class is a subclass of the other class
      const ClassType* current = this->getSuperClass();
      while (current) {
        if (current == classType) {
          return true;
        }
        current = current->superclass;
      }
    }
    return false;
  }

  ClassType *getSuperClass() const {
    return superclass;
  }

  void printImpl(std::ostream &os) const override {
    os << "class " << name;
  }

  friend class TypeContext;
};

class FunctionType;
class Signature : public TypeBase<Signature> {
protected:
  std::vector<Type*> parameters;
  Type* returnType;

  FunctionType *functionType = nullptr;

public:
  Signature(std::vector<Type*> parameters,
            Type* returnType = nullptr)
      : TypeBase("Signature"), parameters(std::move(parameters)), returnType(returnType) {}

  Signature(const Signature &other)
      : TypeBase("Signature"), parameters(other.parameters), returnType(other.returnType) {}

  Type *getParameterType(size_t index) const {
    assert(index < parameters.size() && "Index out of bounds");
    if (index < parameters.size()) {
      return parameters[index];
    }
    return nullptr;
  }

  Type *getReturnType() const {
    return returnType;
  }

  void setReturnType(Type *type) {
    assert(returnType == nullptr && "Return type has already been set");
    returnType = type;
  }

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

  size_t hash() const override {
    size_t seed = 0;
    for (const auto &param : parameters) {
      hash_combine(param->hash(), seed);
    }
    if (returnType) {
      hash_combine(returnType->hash(), seed);
    }
    return seed;
  }

  void printImpl(std::ostream &os) const override {
    os << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
      os << parameters[i]->getName();
      if (i < parameters.size() - 1) {
        os << ", ";
      }
    }
    os << ")";
    if (returnType) {
      os << " -> ";
      os << returnType->getName();
    }
  }

  friend class FunctionType;
};

class FunctionType : public TypeBase<FunctionType> {
protected:
  std::vector<std::unique_ptr<Signature>> overloads;

  FunctionType(std::string name) : TypeBase(name) {}
  FunctionType(std::string name, std::unique_ptr<Signature> signature)
      : TypeBase(name) {
    addOverload(std::move(signature));
  }

public:
  ~FunctionType() override = default;

  const Signature *resolveOverload(const std::vector<Type*> &argTypes) const;

  bool isCompatibleWith(const Type* other) const override {
    assert(false && "Unimplemented FunctionType isCompatibleWith");
    return false;
  }

  std::string getName() const { return name; }

  bool hasOverload(const Signature *signature) const {
    return std::any_of(overloads.begin(), overloads.end(),
                       [&signature](const std::unique_ptr<Signature> &s) {
                        return *s.get() == *signature;
                       });
  }

  void addOverload(std::unique_ptr<Signature> signature) {
    if (hasOverload(signature.get())) {
      ErrorReporter::reportError(
          "Function '" + name + "' already has an overload with the same "
                             "signature.");
      return;
    }
    assert (signature->functionType == nullptr &&
           "Signature already has a function type set");
    signature->functionType = this; // Set the function type for the signature
    overloads.push_back(std::move(signature));
  }

  void addOverload(std::vector<Type*> parameters,
                   Type* returnType = nullptr) {
    auto signature = std::make_unique<Signature>(std::move(parameters), returnType);
    addOverload(std::move(signature));
  }

  bool operator==(const FunctionType *other) const {
    if (other == this) {
      return true;
    }
    if (name != other->name || overloads.size() != other->overloads.size()) {
      return false;
    }
    return false;
  }

  size_t hash() const override {
    size_t seed = Type::hash();
    hash_combine(name, seed);
    for (const auto &overload : overloads) {
      hash_combine(overload->hash(), seed);
    }
    return seed;
  }

  void printImpl(std::ostream &os) const override {
    os << "Function " << name << " with ";
    if (overloads.empty()) {
      os << "no overloads";
      return;
    }
    os << overloads.size() << " overloads ";
  }

  friend class TypeContext;
};
} // namespace lox

#endif // TYPE_H