#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <memory>
#include <string>

#include "Common.h"

namespace lox {
class Type {
protected:
  enum class Kind {
    // Unresolved,
    UnresolvedType,

    // BuiltIn,
    NumberType,
    BoolType,
    StringType,
    NilType,

    // UserDefined,
    ClassType,
    FunctionType,
    ConstructorType,
    InstanceType
  };

public:
  virtual ~Type() = default;

  virtual bool isCompatible(std::shared_ptr<Type> other) = 0;

  virtual Kind getKind() const = 0;
  virtual size_t hash() const;

  virtual bool operator==(const Type *other) const;

  virtual bool operator!=(const Type *other) const { return !(*this == other); }

  virtual void print(std::ostream &os) const = 0;
  virtual void dump() const;
};

// Unresolved types
class UnresolvedType : public Type {
private:
  std::string name;
  inline static std::shared_ptr<UnresolvedType> instance = nullptr;
  UnresolvedType(std::string name) : Type(), name(name) {}

public:
  ~UnresolvedType() override = default;
  static std::shared_ptr<UnresolvedType> getInstance();
  std::string getName() const { return name; }
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  virtual void print(std::ostream &os) const override;

  TYPEID_SYSTEM(Type, UnresolvedType)
};

// BuiltIn types

class NumberType : public Type {
private:
  inline static std::shared_ptr<NumberType> instance = nullptr;
  NumberType() : Type() {}

public:
  ~NumberType() override = default;

  static std::shared_ptr<NumberType> getInstance();
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  virtual void print(std::ostream &os) const override { os << "number"; }

  TYPEID_SYSTEM(Type, NumberType)
};

class StringType : public Type {
private:
  inline static std::shared_ptr<StringType> instance = nullptr;
  StringType() : Type() {}

public:
  ~StringType() override = default;

  static std::shared_ptr<StringType> getInstance();
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  virtual void print(std::ostream &os) const override { os << "string"; }

  TYPEID_SYSTEM(Type, StringType)
};

class BoolType : public Type {
private:
  inline static std::shared_ptr<BoolType> instance = nullptr;
  BoolType() : Type() {}

public:
  ~BoolType() override = default;

  static std::shared_ptr<BoolType> getInstance();
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  virtual void print(std::ostream &os) const override { os << "bool"; }

  TYPEID_SYSTEM(Type, BoolType)
};

class NilType : public Type {
private:
  inline static std::shared_ptr<NilType> instance = nullptr;
  NilType() : Type() {}

public:
  ~NilType() override = default;

  static std::shared_ptr<NilType> getInstance();
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  virtual void print(std::ostream &os) const override { os << "nil"; }

  TYPEID_SYSTEM(Type, NilType)
};

// UserDefined types

class FunctionType : public Type {
public:
  struct Signature {
    std::vector<std::shared_ptr<Type>> parameters;
    std::shared_ptr<Type> returnType;

    Signature(std::vector<std::shared_ptr<Type>> parameters,
              std::shared_ptr<Type> returnType = nullptr)
        : parameters(std::move(parameters)), returnType(std::move(returnType)) {
    }

    bool isResolved() const;
    bool operator==(const Signature &other) const;
    bool operator!=(const Signature &other) const { return !(*this == other); }
    size_t hash() const;
    void print(std::ostream &os) const;
  };

private:
  std::string name;
  std::vector<std::shared_ptr<Signature>> overloads;

public:
  FunctionType(std::string name) : Type(), name(std::move(name)) {}
  FunctionType(std::string name,
               std::vector<std::shared_ptr<Signature>> overloads)
      : FunctionType(std::move(name)) {
    this->overloads = std::move(overloads);
  }
  FunctionType(std::string name, std::vector<std::shared_ptr<Type>> parameters,
               std::shared_ptr<Type> returnType = nullptr)
      : FunctionType(std::move(name)) {
    overloads.emplace_back(std::make_shared<Signature>(std::move(parameters),
                                                       std::move(returnType)));
  }

  ~FunctionType() override = default;

  virtual bool isCompatible(std::shared_ptr<Type> other) override {
    assert_not_reached("Unimplemented FunctionType isCompatible");
  }
  std::string getName() const { return name; }
  bool hasOverload(const Signature &signature) const;
  std::vector<std::shared_ptr<Signature>> getOverloads() const {
    return overloads;
  }
  void addOverload(const Signature &signature);
  void addOverload(const std::vector<std::shared_ptr<Type>> &parameters,
                   std::shared_ptr<Type> returnType = nullptr);
  const std::shared_ptr<Type> getReturnType() const;
  bool operator==(const FunctionType *other) const;
  virtual size_t hash() const override;
  void print(std::ostream &os) const override;

  TYPEID_SYSTEM(Type, FunctionType);
};

class InstanceType;
class ConstructorType : public FunctionType {
public:
  ConstructorType(std::string name,
                  std::vector<std::shared_ptr<Type>> parameters, std::shared_ptr<InstanceType> returnType)
      : FunctionType(std::move(name), std::move(parameters), std::move(std::static_pointer_cast<Type>(returnType))) {}

  ~ConstructorType() override = default;

  TYPEID_SYSTEM(Type, ConstructorType);
};

class ClassType : public Type {
private:
  std::string name;
  std::shared_ptr<ClassType> superClass = nullptr;
  std::unordered_map<std::string, std::shared_ptr<Type>> properties;
  std::shared_ptr<InstanceType> instanceType = nullptr;

public:
  ClassType(const std::string &name,
            std::shared_ptr<ClassType> superClass = nullptr)
      : Type(), name(name), superClass(std::move(superClass)) {}
  ClassType(const ClassType &other)
      : Type(), name(other.name), superClass(other.superClass),
        properties(other.properties) {}

  ~ClassType() override = default;

  virtual ClassType *getSuperClass() const { return superClass.get(); }
  std::shared_ptr<InstanceType> getInstanceType();
  virtual bool isCompatible(std::shared_ptr<Type> other) override;
  bool hasConstructor() const { return properties.find(name) != properties.end(); }
  const std::shared_ptr<ConstructorType> getConstructor();
  virtual const std::shared_ptr<Type> getPropertyType(const std::string &property) const;
  bool addProperty(const std::string &property,
                     std::shared_ptr<Type> type);
  std::string getName() const { return name; }
  bool operator==(const ClassType &other) const { return name == other.name; }
  virtual size_t hash() const override;
  void print(std::ostream &os) const override { os << name; }

  TYPEID_SYSTEM(Type, ClassType);
};

class InstanceType : public ClassType {
private:
  const ClassType *klass;
public:
  InstanceType(ClassType *klass)
      : ClassType(*klass), klass(klass) {}
  ~InstanceType() override = default;
  bool isInstanceOf(const std::shared_ptr<ClassType> &other) const;

  TYPEID_SYSTEM(Type, InstanceType);
};

} // namespace lox

#endif // TYPE_H