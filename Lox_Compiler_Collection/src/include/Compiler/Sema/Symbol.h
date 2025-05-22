#ifndef SYMBOL_H
#define SYMBOL_H

#include "Compiler/AST/Type.h"

namespace lox
{
    struct Symbol {
        std::string name;
        std::shared_ptr<Type> type;
        bool isDefined = false;

        Symbol(const std::string& name, std::shared_ptr<Type> type = nullptr)
            : name(name), type(std::move(type)){}
        Symbol(std::shared_ptr<FunctionType> funcType)
            : name(funcType->getName()), type(std::move(funcType)) {}
        Symbol(std::shared_ptr<ClassType> classType)
            : name(classType->getName()), type(std::move(classType)) {}

        std::string& getName() {
            return name;
        }

        std::shared_ptr<Type>& getType() {
            return type;
        }

        void setType(std::shared_ptr<Type> t) {
            type = t;
        }

        void setDefined() {
            isDefined = true;
        }

        friend std::ostream& operator<<(std::ostream& os, const Symbol& sym) {
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

        virtual void dump() const
        {
            this->print(std::cout);
            std::cout << std::endl;
        }
    };

    // Hash function for Symbol
    struct SymbolHash {
        std::size_t operator()(const Symbol& sym) const {
            std::size_t seed = 0;
            lox::hash_combine(sym.name, seed);
            if (sym.type) {
                lox::hash_combine(sym.type->hash(), seed);
            }
            return seed;
        }
    };
} // namespace lox

#endif // SYMBOL_H