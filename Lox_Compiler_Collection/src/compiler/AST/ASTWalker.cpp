
#include "Compiler/AST/ASTWalker.h"
#include "Compiler/AST/ASTNode.h"

lox::WalkResult lox::Walker::executeCallback(ASTNode* node) {
    auto it = callbacks.find(std::type_index(typeid(*node)));
    if (it != callbacks.end()) {
        return it->second(node);
    }
    return WalkResult::Advance;
}