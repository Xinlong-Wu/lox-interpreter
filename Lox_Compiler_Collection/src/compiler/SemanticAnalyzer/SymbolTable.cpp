#include "Compiler/SemanticAnalyzer/SymbolTable.h"

namespace lox
{
    bool SymbolTable::declare(std::shared_ptr<Symbol> sym) {
        if (this->scopes[this->currentScope].find(sym->name) != this->scopes[this->currentScope].end()) {
            return false; // Variable already defined in this scope
        }
        this->scopes[this->currentScope][sym->name] = std::move(sym);
        return true;
    }

    Symbol* SymbolTable::lookup(const std::string& name) {
        for (int i = this->currentScope; i >= 0; --i) {
            auto it = this->scopes[i].find(name);
            if (it != this->scopes[i].end()) {
                return it->second.get();
            }
        }
        return nullptr; // Variable not found
    }

    void SymbolTable::enterScope() {
        this->scopes.emplace_back();
        this->currentScope++;
    }

    void SymbolTable::exitScope() {
        assert(this->currentScope >= 0 && "No scope to exit");
        this->scopes.pop_back();
        this->currentScope--;
    }
} // namespace lox
