#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "Common.h"
#include "Compiler/Enum.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

namespace lox
{
    #define TYPEID_SYSTEM(className) \
        static bool classof(const Symbol *expr) { return expr->getKind() == Kind::className; } \
        virtual Kind getKind() const override { return Kind::className; }

    // Forward declaration of the SymbolTable class
    class Symbol {
    protected:
        enum class Kind {
            Variable,
            Function,
            Argument,
            Class
        };
    public:
        std::string name;
        lox::Type type;

        Symbol(std::string name, Type type = lox::Type::TYPE_UNKNOWN)
            : name(std::move(name)), type(type){}

        Symbol() = default;
        Symbol(Symbol&&) = default;
        Symbol(const Symbol&) = default;
        Symbol& operator=(const Symbol&) = default;
        Symbol& operator=(Symbol&&) = default;

        virtual bool operator==(const Symbol& other) const {
            return this->slot == other.slot &&
                    this->isFunction == other.isFunction &&
                    this->type == other.type &&
                    this->name == other.name;
        }
        virtual bool operator!=(const Symbol& other) const {
            return !(*this == other);
        }

        friend std::ostream& operator<<(std::ostream& os, const Symbol& sym) {
            sym.print(os);
            return os;
        }

        virtual void print(std::ostream &os) const {
            os << name << ": " << convertTypeToString(type);
        }

        virtual void dump() const {
            this->print(std::cout);
            std::cout << std::endl;
        }

        virtual Kind getKind() const {
            return Kind::Variable;
        }
        static bool classof(const Symbol *expr) { return expr->getKind() == Kind::Variable; }
    };

    class FunctionSymbol : public Symbol {
    private:
        std::vector<lox::Type> parameterTypes;
    public:
        FunctionSymbol(std::string name, std::vector<lox::Type> types, lox::Type returnType = lox::Type::TYPE_NIL)
            : Symbol(std::move(name), returnType), parameterTypes(std::move(types)) {}

        std::string getSignature() const {
            std::string signature = name + "(";
            for (const auto& type : parameterTypes) {
                signature += convertTypeToString(type) + ", ";
            }
            if (!parameterTypes.empty()) {
                signature.erase(signature.size() - 2); // Remove last comma and space
            }
            signature += "): " + convertTypeToString(type);
            return signature;
        }

        size_t getParameterCount() const {
            return parameterTypes.size();
        }

        lox::Type getParameterType(size_t index) const {
            if (index < parameterTypes.size()) {
                return parameterTypes[index];
            }
            return lox::Type::TYPE_UNKNOWN;
        }

        void print(std::ostream &os) const override {
            os << name << " (";
            for (size_t i = 0; i < parameterTypes.size(); ++i) {
                os << convertTypeToString(parameterTypes[i]);
                if (i < parameterTypes.size() - 1) {
                    os << ", ";
                }
            }
            os << "): " << convertTypeToString(type);
        }

        TYPEID_SYSTEM(Function)
    };

    class ArgumentSymbol : public Symbol {
    public:
        ArgumentSymbol(std::string name, lox::Type type = lox::Type::TYPE_UNKNOWN)
            : Symbol(std::move(name), type) {}

        Type getType() const {
            return type;
        }
        void setType(Type type) {
            assert(this->type == lox::Type::TYPE_UNKNOWN && "Type already set");
            assert(type != lox::Type::TYPE_UNKNOWN && "Type cannot be unknown");
            this->type = type;
        }

        void print(std::ostream &os) const override {
            os << "argument " << name << ": " << convertTypeToString(type);
        }

        TYPEID_SYSTEM(Argument)
    };

    class ClassSymbol : public Symbol {
    private:
        ClassSymbol* superClass;
        std::unordered_map<std::string, std::shared_ptr<Symbol>> members;
    public:
        ClassSymbol(std::string name)
            : Symbol(std::move(name), lox::Type::TYPE_OBJECT), superClass(nullptr) {}

        void setSuperClass(ClassSymbol* super) {
            assert(isa<ClassSymbol>(super) && "Super class is not a ClassSymbol");
            superClass = super;
        }
        ClassSymbol* getSuperClass() const {
            return superClass;
        }

        void setMembers(std::unordered_map<std::string, std::shared_ptr<Symbol>> members) {
            assert(this->members.empty() && "Fields already set");
            this->members = std::move(members);
        }

        Symbol* lookupMember(const std::string& name) const {
            auto it = members.find(name);
            if (it != members.end()) {
                return it->second.get();
            }
            if (superClass) {
                return superClass->lookupMember(name);
            }
            return nullptr;
        }

        void print(std::ostream &os) const override {
            os << "class " << name;
        }

        TYPEID_SYSTEM(Class)
    };

    // Hash function for Symbol
    struct SymbolHash {
        std::size_t operator()(const Symbol& sym) const {
            std::size_t seed = 0;
            lox::hash_combine(sym.name, seed);
            lox::hash_combine(sym.type, seed);
            lox::hash_combine(sym.isFunction, seed);
            lox::hash_combine(sym.slot, seed);
            return seed;
        }
    };

    class SymbolTable {
    public:
        void enterScope();
        std::unordered_map<std::string, std::shared_ptr<Symbol>> exitScope();

        bool declare(std::shared_ptr<Symbol> sym);
        bool declareFunction(std::shared_ptr<FunctionSymbol> sym);
        Symbol* lookup(const std::string& name);
        ClassSymbol* lookupClass(const std::string& name);

    private:
        int currentScope = -1;
        std::vector<std::unordered_map<std::string, std::shared_ptr<Symbol>>> scopes;
    };

    #undef TYPEID_SYSTEM
} // namespace lox


#endif // SYMBOLTABLE_H