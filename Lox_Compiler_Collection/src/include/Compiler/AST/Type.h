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

        virtual bool operator==(const Type* other) const {
            if (other == this) {
                return true;
            }
            return this->getKind() == other->getKind();
        }

        virtual bool operator!=(const Type* other) const {
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
        inline static std::shared_ptr<UnresolvedType> instance = nullptr;
        UnresolvedType(std::string name) : Type(), name(name) {}
    public:
        ~UnresolvedType() override = default;

        static std::shared_ptr<UnresolvedType> getInstance() {
            if (!instance) {
                instance = std::shared_ptr<UnresolvedType>(new UnresolvedType("unknown"));
            }
            return instance;
        }

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
    private:
        inline static std::shared_ptr<NumberType> instance = nullptr;
        NumberType() : Type() {}
    public:
        ~NumberType() override = default;

        inline static std::shared_ptr<NumberType> getInstance() {
            if (!instance) {
                instance = std::shared_ptr<NumberType>(new NumberType());
            }
            return instance;
        }

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::NumberType;
        }

        virtual void print(std::ostream &os) const override {
            os << "number";
        }

        TYPEID_SYSTEM(Type, NumberType)
    };

    class StringType : public Type {
    private:
        inline static std::shared_ptr<StringType> instance = nullptr;
        StringType() : Type() {}
    public:
        ~StringType() override = default;

        static std::shared_ptr<StringType> getInstance() {
            if (!instance) {
                instance = std::shared_ptr<StringType>(new StringType());
            }
            return instance;
        }

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::StringType;
        }

        virtual void print(std::ostream &os) const override {
            os << "string";
        }

        TYPEID_SYSTEM(Type, StringType)
    };

    class BoolType : public Type {
    private:
        inline static std::shared_ptr<BoolType> instance = nullptr;
        BoolType() : Type() {}
    public:
        ~BoolType() override = default;

        static std::shared_ptr<BoolType> getInstance() {
            if (!instance) {
                instance = std::shared_ptr<BoolType>(new BoolType());
            }
            return instance;
        }

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return other->getKind() == Kind::BoolType;
        }

        virtual void print(std::ostream &os) const override {
            os << "bool";
        }

        TYPEID_SYSTEM(Type, BoolType)
    };

    class NilType : public Type {
    private:
        inline static std::shared_ptr<NilType> instance = nullptr;
        NilType() : Type() {}
    public:
        ~NilType() override = default;

        static std::shared_ptr<NilType> getInstance() {
            if (!instance) {
                instance = std::shared_ptr<NilType>(new NilType());
            }
            return instance;
        }

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
    public:
        struct Signature {
            std::vector<std::shared_ptr<Type>> parameters;
            std::shared_ptr<Type> returnType;

            Signature(std::vector<std::shared_ptr<Type>> parameters, std::shared_ptr<Type> returnType = nullptr)
                : parameters(std::move(parameters)), returnType(std::move(returnType)) {}

            bool isResolved() const {
                for (const auto& param : parameters) {
                    if (isa<UnresolvedType>(param.get())) {
                        return false;
                    }
                }
                return !isa<UnresolvedType>(returnType.get());
            }

            bool operator==(const Signature& other) const {
                if (parameters.size() != other.parameters.size()) {
                    return false;
                }
                for (size_t i = 0; i < parameters.size(); ++i) {
                    if (parameters[i].get() != other.parameters[i].get()) {
                        return false;
                    }
                }
                return true;
                // return *returnType == *other.returnType;
            }
            bool operator!=(const Signature& other) const {
                return !(*this == other);
            }

            size_t hash() const {
                std::size_t seed = 0;
                lox::hash_combine(returnType->hash(), seed);
                for (const auto& param : parameters) {
                    lox::hash_combine(param->hash(), seed);
                }
                return seed;
            }

            void print(std::ostream &os) const {
                if (!returnType) {
                    os << "constructor";
                }
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
        };

    private:
        std::string name;
        std::vector<std::shared_ptr<Signature>> overloads;
    public:
        FunctionType(std::string name) : Type(), name(std::move(name)) {}
        FunctionType(std::string name, std::vector<std::shared_ptr<Signature>> overloads)
            : FunctionType(std::move(name)) {
                this->overloads = std::move(overloads);
            }
        FunctionType(std::string name, std::vector<std::shared_ptr<Type>> parameters, std::shared_ptr<Type> returnType = nullptr)
            : FunctionType(std::move(name)) {
                overloads.emplace_back(std::make_shared<Signature>(std::move(parameters), std::move(returnType)));
            }

        ~FunctionType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            assert_not_reached("Unimplemented FunctionType isCompatible");
        }

        std::string getName() const {
            return name;
        }

        bool hasOverload(const Signature& signature) const {
            auto overload = std::find_if(overloads.begin(), overloads.end(),
                [&signature](const std::shared_ptr<Signature>& overload) {
                    return *overload == signature;
                });
            return overload != overloads.end();
        }

        void addOverload(const Signature& signature) {
            overloads.push_back(std::make_shared<Signature>(signature));
        }
        void addOverload(const std::vector<std::shared_ptr<Type>>& parameters, std::shared_ptr<Type> returnType = nullptr) {
            overloads.emplace_back(std::make_shared<Signature>(parameters, returnType));
        }

        const std::shared_ptr<Type> getReturnType() const {
            if (!overloads.empty()) {
                return overloads[0]->returnType;
            }
            return nullptr;
        }

        bool operator==(const FunctionType* other) const {
            if (other == this) {
                return true;
            }

            if (name != other->name) {
                return false;
            }

            return std::equal(overloads.begin(), overloads.end(), other->overloads.begin(),
                other->overloads.end(),
                [](const std::shared_ptr<Signature>& a, const std::shared_ptr<Signature>& b) {
                    return *a == *b;
                });
        }

        virtual size_t hash() const override {
            std::size_t seed = Type::hash();
            lox::hash_combine(name, seed);
            for (const auto& overload : overloads) {
                lox::hash_combine(overload->hash(), seed);
            }
            return seed;
        }

        void print(std::ostream &os) const override {
            os << " -> ";
            getReturnType()->print(os);
            os << " overloads: ";
        }

        TYPEID_SYSTEM(Type, FunctionType);
    };

    class ClassType : public Type {
    private:
        std::string name;
        std::shared_ptr<ClassType> superClass;
        std::unordered_map<std::string, std::shared_ptr<FunctionType>> methods;
    public:
        ClassType(const std::string& name, std::shared_ptr<ClassType> superClass = nullptr)
            : Type(), name(name), superClass(std::move(superClass)){
            if (!hasConstructor()) {
                // Automatically add a constructor if it doesn't exist
                methods[name] = std::make_shared<FunctionType>(name, std::vector<std::shared_ptr<Type>>());
            }
        }

        ~ClassType() override = default;

        virtual bool isCompatible(std::shared_ptr<Type> other) override {
            return isa<ClassType>(other) &&
                   (*this == *dyn_cast<ClassType>(other));

        }

        bool hasConstructor() const {
            return methods.find("Foo") != methods.end();
        }

        const std::shared_ptr<FunctionType> getConstructor() {
            auto it = methods.find("Foo");
            if (it != methods.end()) {
                return it->second;
            }
            return nullptr;
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