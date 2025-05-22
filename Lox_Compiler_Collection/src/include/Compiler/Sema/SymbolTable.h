#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include "Compiler/Sema/Scope.h"

namespace lox
{
    class SymbolTable {
    private:
        std::vector<std::shared_ptr<Scope>> scopes;
    public:
        SymbolTable() {
            scopes.push_back(std::make_shared<GlobalScope>());
        }
        ~SymbolTable() = default;

        void enterScope(const std::string& name = "anonymous") {
            scopes.push_back(std::make_shared<Scope>(scopes.back(), name));
        }

        void exitScope() {
            assert(!scopes.empty() && "No scope to exit");
            scopes.pop_back();
        }

        std::shared_ptr<Scope> getCurrentScope() const {
            return scopes.back();
        }

        bool declare(std::shared_ptr<Symbol> sym) {
            return scopes.back()->declare(sym);
        }

        std::shared_ptr<Symbol> lookupSymbol(const std::string& name) {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto sym = (*it)->resolve(name);
                if (sym) {
                    return sym;
                }
            }
            return nullptr;
        }

        void print(std::ostream &os) const {
            for (size_t i = 0; i < scopes.size(); i++) {
                scopes[i]->print(os, i);
            }
        }
        virtual void dump() const {
            this->print(std::cout);
            std::cout << std::endl;
        }
    };
} // namespace lox


#endif // SYMBOLTABLE_H