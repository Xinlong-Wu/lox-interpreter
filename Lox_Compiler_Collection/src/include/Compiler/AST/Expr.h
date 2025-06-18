#ifndef EXPR_H
#define EXPR_H

#include <iostream>
#include <memory>
#include <vector>

#include "Common.h"
#include "Compiler/AST/ASTNode.h"
#include "Compiler/AST/ASTVisitor.h"
#include "Compiler/AST/Type.h"
#include "Compiler/Scanner/Token.h"
// #include "Compiler/Sema/Symbol.h"

namespace lox {

class ExprBase : public ASTNode {
protected:
  Location loc;
  std::shared_ptr<Type> type = nullptr;
public:
  ExprBase(Location loc) : loc(loc) {}
  virtual ~ExprBase() = default;

  const Location& getLoc() const { return loc; }
  const std::shared_ptr<Type> getType() const {
    return type;
  }

  virtual ClassID getClassID() const = 0;

  virtual void print(std::ostream &os) const = 0;
  virtual void dump() const {
    print(std::cout);
    std::cout << std::endl;
  }
};

template<typename Derived>
class ExprCRTP : public ExprBase {
protected:
  // 存储实际的类型ID
  ClassID classID;

  ExprCRTP(Location loc)
      : ExprBase(loc), classID(getClassIdOf<Derived>()) {}
public:
  ClassID getClassID() const override { return classID; }

  static bool classof(const ExprBase* expr) {
    return expr->getClassID() == getClassIdOf<Derived>();
  }

  void print(std::ostream &os) const override {
    cast<Derived>(this)->printImpl(os);
  }

  void accept(ASTVisitor &visitor) override {
    visitor.visit(static_cast<Derived&>(*this));
  }
};

// Literal expression
class NumberExpr : public ExprCRTP<NumberExpr> {
  double value;
public:
  NumberExpr(double value, const Location &loc)
      : ExprCRTP(loc), value(value){}

  void printImpl(std::ostream &os) const {
    os << value;
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

class StringExpr : public ExprCRTP<StringExpr> {
  std::string value;
public:
  StringExpr(const std::string &value, const Location &loc)
      : ExprCRTP(loc), value(value){}
  StringExpr(std::string_view value, const Location &loc)
      : ExprCRTP(loc), value(value) {}

  void printImpl(std::ostream &os) const {
    os << '"' << value << '"';
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

class BoolExpr : public ExprCRTP<BoolExpr> {
  bool value;
public:
  BoolExpr(bool value, const Location &loc)
      : ExprCRTP(loc), value(value) {}

  void printImpl(std::ostream &os) const {
    os << (value ? "true" : "false");
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

class NilExpr : public ExprCRTP<NilExpr> {
public:
  NilExpr(const Location &loc)
      : ExprCRTP(loc) {}

  void printImpl(std::ostream &os) const {
    os << "nil";
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

// Variable and access expressions
class VariableExpr : public ExprCRTP<VariableExpr> {
  std::string name;
public:
  VariableExpr(const std::string &name, const Location &loc)
      : ExprCRTP(loc), name(name) {}
  VariableExpr(std::string_view name, const Location &loc)
      : ExprCRTP(loc), name(name) {}

  void printImpl(std::ostream &os) const {
    os << name;
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

class AccessExpr : public ExprCRTP<AccessExpr> {
  std::unique_ptr<ExprBase> base;
  std::string property;
public:
  AccessExpr(std::unique_ptr<ExprBase> base, const std::string &property, const Location &loc)
      : ExprCRTP(loc), base(std::move(base)), property(property) {}

  void printImpl(std::ostream &os) const {
    base->print(os);
    os << "." << property;
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkOrder order = walker.getOrder();

    if (order == WalkOrder::PreOrder) {
      WalkResult result = walker.executeCallback(this);
      if (result == WalkResult::Skip) return WalkResult::Advance;
      if (result == WalkResult::Interrupt) return result; // Interrupt the walk
    }

    WalkResult result = base->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    if (order == WalkOrder::PostOrder) {
      result = walker.executeCallback(this);
    }

    return result == WalkResult::Skip ? WalkResult::Advance : result; // Continue with the next node
  }
};


// operation expressions
class UnaryExpr : public ExprCRTP<UnaryExpr> {
public:
  enum class Op { Negate, Not };
protected:
  std::unique_ptr<ExprBase> operand;
  Op op;
public:
  UnaryExpr(Op op, std::unique_ptr<ExprBase> operand, const Location &loc)
      : ExprCRTP(loc), operand(std::move(operand)), op(op) {}

  void printImpl(std::ostream &os) const {
    os << toString(op);
    operand->print(os);
  }

  static std::string toString(Op op) {
    switch (op) {
      case Op::Negate: return "-";
      case Op::Not: return "!";
      default: return "<unknown>";
    }
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkOrder order = walker.getOrder();

    if (order == WalkOrder::PreOrder) {
      WalkResult result = walker.executeCallback(this);
      if (result == WalkResult::Skip) return WalkResult::Advance;
      if (result == WalkResult::Interrupt) return result; // Interrupt the walk
    }

    WalkResult result = operand->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    if (order == WalkOrder::PostOrder) {
      result = walker.executeCallback(this);
    }

    if (result == WalkResult::Skip) {
      return WalkResult::Advance; // Skip children but continue with siblings
    }
    return result; // Continue with the next node
  }
};

class BinaryExpr : public ExprCRTP<BinaryExpr> {
public:
  enum class Op { Add, Sub, Mul, Div, Mod, And, Or, Equal, NotEqual, GreaterThan, GreaterThanEqual };
  std::unique_ptr<ExprBase> left;
  std::unique_ptr<ExprBase> right;
  Op op;
public:
  BinaryExpr(Op op, std::unique_ptr<ExprBase> left, std::unique_ptr<ExprBase> right, const Location &loc)
      : ExprCRTP(loc), left(std::move(left)), right(std::move(right)), op(op) {}

  void printImpl(std::ostream &os) const {
    left->print(os);
    os << " " << toString(op) << " ";
    right->print(os);
  }

  static std::string toString(Op op) {
    switch (op) {
      case Op::Add: return "+";
      case Op::Sub: return "-";
      case Op::Mul: return "*";
      case Op::Div: return "/";
      case Op::Mod: return "%";
      case Op::And: return "&&";
      case Op::Or: return "||";
      case Op::Equal: return "==";
      case Op::NotEqual: return "!=";
      case Op::GreaterThan: return ">";
      case Op::GreaterThanEqual: return ">=";
      default: return "<unknown>";
    }
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkOrder order = walker.getOrder();

    if (order == WalkOrder::PreOrder) {
      WalkResult result = walker.executeCallback(this);
      if (result == WalkResult::Skip) return WalkResult::Advance;
      if (result == WalkResult::Interrupt) return result; // Interrupt the walk
    }

    WalkResult result = left->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    result = right->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    if (order == WalkOrder::PostOrder) {
      result = walker.executeCallback(this);
    }

    if (result == WalkResult::Skip) {
      return WalkResult::Advance; // Skip children but continue with siblings
    }
    return result; // Continue with the next node
  }
};

class AssignExpr : public ExprCRTP<AssignExpr> {
  std::unique_ptr<ExprBase> left;
  std::unique_ptr<ExprBase> right;
public:
  AssignExpr(std::unique_ptr<ExprBase> left, std::unique_ptr<ExprBase> right, const Location &loc)
      : ExprCRTP(loc), left(std::move(left)), right(std::move(right)) {}

  void printImpl(std::ostream &os) const {
    left->print(os);
    os << " = ";
    right->print(os);
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkOrder order = walker.getOrder();

    if (order == WalkOrder::PreOrder) {
      WalkResult result = walker.executeCallback(this);
      if (result == WalkResult::Skip) return WalkResult::Advance;
      if (result == WalkResult::Interrupt) return result; // Interrupt the walk
    }

    WalkResult result = left->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    result = right->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    if (order == WalkOrder::PostOrder) {
      result = walker.executeCallback(this);
    }

    if (result == WalkResult::Skip) {
      return WalkResult::Advance; // Skip children but continue with siblings
    }
    return result; // Continue with the next node
  }
};

class CallExpr : public ExprCRTP<CallExpr> {
  std::unique_ptr<ExprBase> callee;
  std::vector<std::unique_ptr<ExprBase>> arguments;
public:
  CallExpr(std::unique_ptr<ExprBase> callee, std::vector<std::unique_ptr<ExprBase>> arguments, const Location &loc)
      : ExprCRTP(loc), callee(std::move(callee)), arguments(std::move(arguments)) {}

  void printImpl(std::ostream &os) const {
    callee->print(os);
    os << "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
      if (i > 0) os << ", ";
      arguments[i]->print(os);
    }
    os << ")";
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkOrder order = walker.getOrder();

    if (order == WalkOrder::PreOrder) {
      WalkResult result = walker.executeCallback(this);
      if (result == WalkResult::Skip) return WalkResult::Advance;
      if (result == WalkResult::Interrupt) return result; // Interrupt the walk
    }

    WalkResult result = callee->walkInternal(walker);
    if (result == WalkResult::Interrupt) return result;

    for (const auto &arg : arguments) {
      result = arg->walkInternal(walker);
      if (result == WalkResult::Interrupt) return result;
    }

    if (order == WalkOrder::PostOrder) {
      result = walker.executeCallback(this);
    }

    if (result == WalkResult::Skip) {
      return WalkResult::Advance; // Skip children but continue with siblings
    }
    return result; // Continue with the next node
  }
};

class ParameterExpr : public ExprCRTP<ParameterExpr> {
protected:
  std::string name;
  std::optional<std::string> typeName = std::nullopt;
  // std::unique_ptr<ExprBase> defaultValue;
public:
  ParameterExpr(const std::string &name, const Location &loc)
      : ExprCRTP(loc), name(name){}
  ParameterExpr(const std::string &name, const std::string &typeName, const Location &loc)
      : ExprCRTP(loc), name(name), type(type) {}

  std::optional<std::string> getTypeName() const {
    return typeName;
  }

  void printImpl(std::ostream &os) const {
    os << name;
    if (type) {
      os << ": " << type;
    }
  }

  WalkResult walkInternal(Walker& walker) override {
    WalkResult result = walker.executeCallback(this);
    return result == WalkResult::Skip ? WalkResult::Advance : result;
  }
};

} // namespace lox
#endif // EXPR_H