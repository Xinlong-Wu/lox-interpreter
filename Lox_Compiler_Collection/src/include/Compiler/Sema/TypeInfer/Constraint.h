#ifndef CONSTRAINT_H
#define CONSTRAINT_H

namespace lox
{
class Constraint
{
public:
    // Constraint 类型的枚举
    enum class ConstraintType
    {
        ASSIGNABLE, // 可赋值
        EQUAL,      // 相等
    };
private:
    /* data */
    const Type* lType; // 左侧类型
    const Type* rType; // 右侧类型
    ConstraintType relation; // 约束关系
public:
    Constraint(const Type* left, const Type* right, ConstraintType rel)
        : lType(left), rType(right), relation(rel) {}
    ~Constraint();
};
} // namespace lox


#endif // CONSTRAINT_H