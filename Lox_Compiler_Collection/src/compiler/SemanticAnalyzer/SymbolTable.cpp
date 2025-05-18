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

    ClassSymbol* SymbolTable::lookupClass(const std::string& name) {
        Symbol* sym = this->lookup(name);
        if (sym && isa<ClassSymbol>(sym)) {
            return static_cast<ClassSymbol*>(sym);
        }
        return nullptr; // Class not found
    }

    void SymbolTable::enterScope() {
        this->scopes.emplace_back();
        this->currentScope++;
    }

    std::unordered_map<std::string, std::shared_ptr<Symbol>> SymbolTable::exitScope() {
        assert(!scopes.empty() && "No scope to exit");
        std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols = std::move(this->scopes.back());
        this->scopes.pop_back();
        this->currentScope--;
        return symbols;
    }
} // namespace lox
