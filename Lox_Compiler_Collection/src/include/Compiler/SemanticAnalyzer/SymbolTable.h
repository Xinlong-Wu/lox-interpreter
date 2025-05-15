#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

namespace lox
{
    // Forward declaration of the SymbolTable class
    struct Symbol {
        std::string name;
        std::string type;
        Type type;
        bool isFunction;
    };
} // namespace lox


#endif // SYMBOLTABLE_H