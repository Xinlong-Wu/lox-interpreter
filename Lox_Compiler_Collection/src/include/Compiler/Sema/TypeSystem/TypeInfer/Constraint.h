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
    Type* lType; // 左侧类型
    Type* rType; // 右侧类型
    ConstraintType relation; // 约束关系
public:
    Constraint(Type* left, Type* right, ConstraintType rel)
        : lType(left), rType(right), relation(rel) {}
    ~Constraint();

    Type* getLeftType() const { return lType; }
    Type* getRightType() const { return rType; }
    ConstraintType getRelation() const { return relation; }
};
} // namespace lox


#endif // CONSTRAINT_H