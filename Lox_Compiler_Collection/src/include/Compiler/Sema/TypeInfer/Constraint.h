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
        EQUAL,      // 等于
        NOT_EQUAL,  // 不等于
        SUBTYPE,    // 子类型
        SUPER_TYPE, // 超类型
        UNKNOWN     // 未知类型
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