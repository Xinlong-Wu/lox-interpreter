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

  // 新增的walker接口 - 模板版本，直接传入lambda（返回WalkResult）
  template<typename T>
  WalkResult walk(WalkCallback<T> callback, WalkOrder order = WalkOrder::PreOrder) {
      Walker walker(order);
      walker.registerCallback<T>(callback);
      return walkInternal(walker);
  }

  // 新增的walker接口 - 模板版本，直接传入lambda（无返回值）
  template<typename T>
  WalkResult walk(VoidWalkCallback<T> callback, WalkOrder order = WalkOrder::PreOrder) {
      Walker walker(order);
      walker.registerCallback<T>(callback);
      return walkInternal(walker);
  }

  // 新增的walker接口 - Walker对象版本，可以注册多个类型的回调
  WalkResult walk(Walker& walker) {
      return walkInternal(walker);
  }

  virtual WalkResult walkInternal(Walker& walker) = 0;
};
} // namespace lox

#endif // ASTNODE_H