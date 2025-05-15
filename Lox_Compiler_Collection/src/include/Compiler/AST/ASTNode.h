#ifndef ASTNODE_H
#define ASTNODE_H

namespace lox
{
    class ASTVisitor;

    class ASTNode {
    public:
        virtual ~ASTNode() = default;
        virtual void accept(ASTVisitor& visitor) = 0;
    };
} // namespace lox

#endif // ASTNODE_H