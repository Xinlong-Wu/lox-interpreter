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
            Class
        };
    public:
        std::string name;
        lox::Type type;
        bool isFunction;
        int slot;

        Symbol(std::string name, Type type = lox::Type::TYPE_UNKNOWN, bool isFunction = false)
            : name(std::move(name)), type(type), isFunction(isFunction), slot(0) {}

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
            : Symbol(std::move(name), returnType, true), parameterTypes(std::move(types)) {}
        
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
        void exitScope();

        bool declare(std::shared_ptr<Symbol> sym);
        Symbol* lookup(const std::string& name);

    private:
        int currentScope = -1;
        std::vector<std::unordered_map<std::string, std::shared_ptr<Symbol>>> scopes;
    };

    #undef TYPEID_SYSTEM
} // namespace lox


#endif // SYMBOLTABLE_H