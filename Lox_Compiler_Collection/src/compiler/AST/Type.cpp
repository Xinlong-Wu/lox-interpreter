#include "Compiler/AST/Type.h"

#include <iostream>
#include <memory>
#include <string>

using namespace std;
using namespace lox;

size_t Type::hash() const {
    size_t seed = 0;
    hash_combine(static_cast<underlying_type_t<Type::Kind>>(getKind()),
                      seed);
    return seed;
}

bool Type::operator==(const Type *other) const {
    if (other == this) {
      return true;
    }
    return this->getKind() == other->getKind();
}

void Type::dump() const {
    this->print(std::cout);
    std::cout << std::endl;
}

shared_ptr<UnresolvedType> UnresolvedType::getInstance() {
    if (!instance) {
        instance = shared_ptr<UnresolvedType>(new UnresolvedType("unknown"));
    }
    return instance;
}

bool UnresolvedType::isCompatible(shared_ptr<Type> other) {
    // Unresolved types are not compatible with any other type
    return false;
}

void UnresolvedType::print(ostream &os) const {
    os << name << "(unresolved)";
}

shared_ptr<NumberType> NumberType::getInstance() {
    if (!instance) {
        instance = shared_ptr<NumberType>(new NumberType());
    }
    return instance;
}

bool NumberType::isCompatible(shared_ptr<Type> other) {
    return isa<NumberType>(other);
}

shared_ptr<StringType> StringType::getInstance() {
    if (!instance) {
        instance = shared_ptr<StringType>(new StringType());
    }
    return instance;
}

bool StringType::isCompatible(shared_ptr<Type> other) {
    return isa<StringType>(other);
}

shared_ptr<BoolType> BoolType::getInstance() {
    if (!instance) {
        instance = shared_ptr<BoolType>(new BoolType());
    }
    return instance;
}

bool BoolType::isCompatible(shared_ptr<Type> other) {
    return isa<BoolType>(other);
}

shared_ptr<NilType> NilType::getInstance() {
    if (!instance) {
        instance = shared_ptr<NilType>(new NilType());
    }
    return instance;
}

bool NilType::isCompatible(shared_ptr<Type> other) {
    return isa<NilType>(other);
}


// ================ UserDefined Types ================

bool FunctionType::Signature::isResolved() const {
    for (const auto &param : parameters) {
        if (isa<UnresolvedType>(param.get())) {
            return false;
        }
    }
    return !isa<UnresolvedType>(returnType.get());
}

bool FunctionType::Signature::operator==(const FunctionType::Signature &other) const {
    if (parameters.size() != other.parameters.size()) {
        return false;
    }
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (parameters[i].get() != other.parameters[i].get()) {
            return false;
        }
    }
    // if (returnType != other.returnType) {
    //     return false;
    // }
    return true;
}

size_t FunctionType::Signature::hash() const {
    std::size_t seed = 0;
    lox::hash_combine(returnType->hash(), seed);
    for (const auto &param : parameters) {
        lox::hash_combine(param->hash(), seed);
    }
    return seed;
}

void FunctionType::Signature::print(ostream &os) const {
    os << "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        parameters[i]->print(os);
        if (i < parameters.size() - 1) {
            os << ", ";
        }
    }
    os << ")";
    if (returnType) {
        os << " -> ";
        returnType->print(os);
    }
}

bool FunctionType::hasOverload(const FunctionType::Signature &signature) const {
    auto overload = std::find_if(overloads.begin(), overloads.end(),
                                 [&signature](const std::shared_ptr<Signature> &overload) {
                                     return *overload == signature;
                                 });
    return overload != overloads.end();
}

void FunctionType::addOverload(const FunctionType::Signature &signature) {
    overloads.push_back(std::make_shared<Signature>(signature));
}

void FunctionType::addOverload(const std::vector<std::shared_ptr<Type>> &parameters,
                                 std::shared_ptr<Type> returnType) {
    overloads.emplace_back(std::make_shared<Signature>(parameters, returnType));
}

const std::shared_ptr<Type> FunctionType::getReturnType() const {
    if (!overloads.empty()) {
        return overloads[0]->returnType;
    }
    return nullptr;
}

bool FunctionType::operator==(const FunctionType *other) const {
    if (other == this) {
        return true;
    }

    if (name != other->name) {
        return false;
    }

    return std::equal(
        overloads.begin(), overloads.end(), other->overloads.begin(),
        other->overloads.end(),
        [](const std::shared_ptr<Signature> &a,
           const std::shared_ptr<Signature> &b) { return *a == *b; });
}

size_t FunctionType::hash() const {
    std::size_t seed = Type::hash();
    lox::hash_combine(name, seed);
    for (const auto &overload : overloads) {
        lox::hash_combine(overload->hash(), seed);
    }
    return seed;
}

void FunctionType::print(ostream &os) const {
    os << "Function " << name << " with ";
    os << overloads.size() << " overloads";
    // for (const auto &overload : overloads) {
    //     overload->print(os);
    //     os << "\n";
    // }
}

bool ClassType::isCompatible(shared_ptr<Type> other) {
    if (isa<ClassType>(other)) {
        return *this == *dyn_cast<ClassType>(other);
    }
    return false;
}

const std::shared_ptr<ConstructorType> ClassType::getConstructor() {
    auto it = properties.find(name);
    if (it != properties.end()) {
        return dyn_cast<ConstructorType>(it->second);
    }
    return nullptr;
}

shared_ptr<InstanceType> ClassType::getInstanceType() {
    if (!instanceType) {
        instanceType = make_shared<InstanceType>(this);
    }
    return instanceType;
}

const std::shared_ptr<Type> ClassType::getPropertyType(const std::string &property) const {
    auto it = properties.find(property);
    if (it != properties.end()) {
        return it->second;
    }
    if (superClass) {
        return superClass->getPropertyType(property);
    }
    // ErrorReporter::reportError(
    //     "Property '" + property + "' not found in class '" + name + "'");
    assert_not_reached(
        "Property not found in class, should have been caught earlier");
}

bool ClassType::addProperty(const std::string &property,
                            shared_ptr<Type> type) {
    if (properties.find(property) != properties.end()) {
        // ErrorReporter::reportError("Property '" + property +
        //                            "' already exists in class '" + name + "'");
        assert_not_reached("Property already exists in class, should have been caught earlier");
        return false;
    }
    properties[property] = std::move(type);
    return true;
}

size_t ClassType::hash() const {
    std::size_t seed = Type::hash();
    lox::hash_combine(name, seed);
    if (superClass) {
        lox::hash_combine(superClass->hash(), seed);
    }
    for (const auto &property : properties) {
        lox::hash_combine(property.first, seed);
        lox::hash_combine(property.second->hash(), seed);
    }
    return seed;
}

bool InstanceType::isInstanceOf(const std::shared_ptr<ClassType> &other) const {
    const ClassType *currentClass = klass;
    while (currentClass) {
        if (*currentClass == *other) {
            return true;
        }
        currentClass = currentClass->getSuperClass();
    }
    return false;
}
