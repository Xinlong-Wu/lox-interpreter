#ifndef TYPE_H
#define TYPE_H

#include "Common.h"

#include <iostream>
#include <memory>
#include <string>

namespace lox
{
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
        };
    public:
        virtual ~Type() = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) = 0;

        virtual Kind getKind() const = 0;
        virtual size_t hash() const {
            std::size_t seed = 0;
            lox::hash_combine(static_cast<std::underlying_type_t<Kind>>(getKind()), seed);
            return seed;
        }

        virtual bool operator==(Type& other) const {
            return this->getKind() == other.getKind();
        }

        virtual bool operator!=(Type& other) const {
            return !(*this == other);
        }

        virtual void print(std::ostream &os) const = 0;
        virtual void dump() const
        {
            this->print(std::cout);
            std::cout << std::endl;
        }
    };

    // Unresolved types
    class UnresolvedType : public Type {
    private:
        std::string name;
    public:
        UnresolvedType(std::string name) : Type(), name(name) {}
        ~UnresolvedType() override = default;

        std::string getName() const {
            return name;
        }

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return false;
        }

        virtual void print(std::ostream &os) const override {
            os << name <<"(unresolved)";
        }

        TYPEID_SYSTEM(Type, UnresolvedType)
    };

    // BuiltIn types

    class NumberType : public Type {
    public:
        NumberType() : Type() {}
        ~NumberType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::NumberType;
        }

        virtual void print(std::ostream &os) const override {
            os << "number";
        }

        TYPEID_SYSTEM(Type, NumberType)
    };

    class StringType : public Type {
    public:
        StringType() : Type() {}
        ~StringType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::StringType;
        }

        virtual void print(std::ostream &os) const override {
            os << "string";
        }

        TYPEID_SYSTEM(Type, StringType)
    };

    class BoolType : public Type {
    public:
        BoolType() : Type() {}
        ~BoolType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::BoolType;
        }

        virtual void print(std::ostream &os) const override {
            os << "bool";
        }

        TYPEID_SYSTEM(Type, BoolType)
    };

    class NilType : public Type {
    public:
        NilType() : Type() {}
        ~NilType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::NilType;
        }

        virtual void print(std::ostream &os) const override {
            os << "nil";
        }

        TYPEID_SYSTEM(Type, NilType)
    };

    // UserDefined types

    class FunctionType : public Type {
    private:
        std::string name;
        std::vector<std::shared_ptr<Type>> parameters;
        std::shared_ptr<Type> returnType;
    public:
        FunctionType(std::string name, std::vector<std::shared_ptr<Type>> parameters, std::shared_ptr<Type> returnType = nullptr)
            : Type(), name(std::move(name)), parameters(std::move(parameters)), returnType(std::move(returnType)) {}

        ~FunctionType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            assert_not_reached("Unimplemented FunctionType isCompatible");
        }

        std::string getName() const {
            return name;
        }

        std::shared_ptr<Type>& getParameter(size_t index) {
            assert(index < parameters.size());
            return parameters[index];
        }

        std::shared_ptr<Type>& getReturnType() const {
            return returnType;
        }

        bool operator==(const FunctionType& other) const {
            if (name != other.name) {
                return false;
            }
            if (parameters.size() != other.parameters.size()) {
                return false;
            }
            for (size_t i = 0; i < parameters.size(); ++i) {
                if (*parameters[i] != *other.parameters[i]) {
                    return false;
                }
            }
            return *returnType == *other.returnType;
        }

        virtual size_t hash() const override {
            std::size_t seed = Type::hash();
            lox::hash_combine(name, seed);
            for (const auto& param : parameters) {
                lox::hash_combine(param->hash(), seed);
            }
            lox::hash_combine(returnType->hash(), seed);
            return seed;
        }

        void print(std::ostream &os) const override {
            os << "(";
            for (size_t i = 0; i < parameters.size(); ++i) {
                parameters[i]->print(os);
                if (i < parameters.size() - 1) {
                    os << ", ";
                }
            }
            os << ") -> ";
            returnType->print(os);
        }

        TYPEID_SYSTEM(Type, FunctionType);
    };

    class ClassType : public Type {
    private:
        std::string name;
        std::shared_ptr<ClassType> superClass;
        // std::unordered_map<std::string, std::shared_ptr<Type>> methods;
    public:
        ClassType(std::string& name, std::shared_ptr<ClassType> superClass = nullptr)
            : Type(), name(name), superClass(std::move(superClass)){}

        ~ClassType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            assert_not_reached("Unimplemented ClassType isCompatible");
        }

        std::string getName() const {
            return name;
        }

        bool operator==(const ClassType& other) const {
            return name == other.name;
        }

        virtual size_t hash() const override {
            std::size_t seed = Type::hash();
            lox::hash_combine(name, seed);
            if (superClass) {
                lox::hash_combine(superClass->hash(), seed);
            }
            return seed;
        }

        void print(std::ostream &os) const override {
            os << name;
        }

        TYPEID_SYSTEM(Type, ClassType);
    };

} // namespace lox


#endif // TYPE_H