#ifndef SCOPE_H
#define SCOPE_H

#include "Common.h"
#include  "Compiler/Sema/Symbol.h"

namespace lox
{
    class Scope {
    protected:
        enum class Kind {
            GlobalScope,
            ClassScope,
            FunctionScope,
            BlockScope
        };

        std::string name;
        std::unordered_map<std::string, std::shared_ptr<Symbol>> symbols;
        std::shared_ptr<Scope> enclosingScope;      // 外层作用域
    public:
        Scope(std::shared_ptr<Scope> parent, const std::string& name)
            : enclosingScope(parent), name(name) {}
        virtual ~Scope() = default;

        bool declare(std::shared_ptr<Symbol>& symbol) {
            if (symbols.find(symbol->getName()) != symbols.end()) {
                return false; // Symbol already declared in this scope
            }
            symbols[symbol->getName()] = symbol;
            return true;
        }

        std::shared_ptr<Symbol> resolve(const std::string& name) {
            auto it = symbols.find(name);
            if (it != symbols.end()) {
                return it->second;
            }

            // 如果当前作用域没有找到，尝试外层作用域
            return enclosingScope ? enclosingScope->resolve(name) : nullptr;
        }

        std::shared_ptr<Scope> getEnclosingScope() const {
            return enclosingScope;
        }

        const std::string& getName() const {
            return name;
        }

        void print(std::ostream& os, int level = 0) const {
            std::string indent(level * 2, '\t');
            os << indent << "Scope: " << name << std::endl;

            for (const auto& [name, symbol] : symbols) {
                os << indent;
                symbol->print(os);
                os << std::endl;
            }

            if (enclosingScope) {
                enclosingScope->print(os, level + 1);
            }
        }

        void dump(int level = 0) const {
            print(std::cout, level);
            std::cout << std::endl;
        }

        static bool classof(const Scope *ptr) { return ptr->getKind() == Kind::BlockScope; } \
        static bool classof(const std::shared_ptr<Scope> ptr) { return Scope::classof(ptr.get()); } \
        virtual Kind getKind() const { return Kind::BlockScope; }
    };

    class GlobalScope : public Scope {
    public:
        GlobalScope() : Scope(nullptr, "Global") {}

        TYPEID_SYSTEM(Scope, GlobalScope)
    };

    class ClassScope : public Scope {
    public:
        ClassScope(std::shared_ptr<Scope>& parent, const std::string& name)
            : Scope(parent, name) {}

        TYPEID_SYSTEM(Scope, ClassScope)
    };

    class FunctionScope : public Scope {
    public:
        FunctionScope(std::shared_ptr<Scope>& parent, const std::string& name)
            : Scope(parent, name) {}

        TYPEID_SYSTEM(Scope, FunctionScope)
    };

    // class BlockScope : public Scope {
    // public:
    //     BlockScope(std::shared_ptr<Scope>& parent, const std::string& name)
    //         : Scope(parent, name) {}

    //     TYPEID_SYSTEM(Scope, BlockScope)
    // };
} // namespace lox


#endif // SCOPE_H