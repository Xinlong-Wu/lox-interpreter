#ifndef STMT_H
#define STMT_H

namespace lox
{
    class StmtBase
    {
    public:
        virtual ~StmtBase() = default;
        virtual void dump() const = 0;
    };
} // namespace lox


#endif // STMT_H