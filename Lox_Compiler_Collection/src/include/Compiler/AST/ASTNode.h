#ifndef ASTNODE_H
#define ASTNODE_H

#include "Compiler/AST/ASTWalker.h"

#define ACCEPT_DECL() void accept(ASTVisitor &visitor) override

namespace lox {
class ASTVisitor;
class ASTNode {
public:
  virtual ~ASTNode() = default;
  virtual void accept(ASTVisitor &visitor) = 0;

  virtual bool isExpression() const { return false; }
  virtual bool isStatement() const { return false; }

  template <typename T, typename Callable>
  WalkResult walk(Callable &&callback, WalkOrder order = WalkOrder::PreOrder) {
    Walker walker(order);

    if constexpr (std::is_same_v<std::invoke_result_t<Callable, T *>,
                                 WalkResult>) {
      // 如果lambda返回WalkResult，直接使用
      walker.registerCallback<T>(std::forward<Callable>(callback));
    } else {
      // 如果lambda返回void或其他类型，包装成返回WalkResult::Advance的形式
      walker.registerCallback<T>(
          [callback = std::forward<Callable>(callback)](T *node) -> WalkResult {
            callback(node);
            return WalkResult::Advance;
          });
    }

    return walkInternal(walker);
  }

  // 新增的walker接口 - Walker对象版本，可以注册多个类型的回调
  WalkResult walk(Walker &walker) { return walkInternal(walker); }

  virtual WalkResult walkInternal(Walker &walker) = 0;
};
} // namespace lox

#endif // ASTNODE_H
