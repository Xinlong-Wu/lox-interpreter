#ifndef SYMBOL_H
#define SYMBOL_H

namespace lox
{
    struct Symbol {
        std::string name;
        std::shared_ptr<Type::Type> type;

        Symbol(const std::string& name, std::shared_ptr<Type::Type> type = nullptr)
            : name(name), type(std::move(type)){}

        std::string& getName() {
            return name;
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

        void dump() const {
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