#ifndef STMT_H
#define STMT_H

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common.h"
#include "Compiler/AST/ASTNode.h"
#include "Compiler/AST/Expr.h"
#include "Compiler/Sema/Scope.h"
#include "Compiler/Sema/Symbol.h"
#include "Compiler/Location.h"

namespace lox {
class StmtBase : public ASTNode {
public:
  using ClassID = const void*;

  virtual ~StmtBase() = default;

  virtual const Location &getLoc() const = 0;

  virtual ClassID getClassID() const = 0;

  virtual void print(std::ostream &os) const = 0;
  virtual void dump() const {
    this->print(std::cout);
    std::cout << std::endl;
  }

  virtual void accept(ASTVisitor &visitor) = 0;
};

template<typename Derived>
class StmtCRTP : public StmtBase {
protected:
  ClassID classID;
  template<typename T>
  static ClassID _getClassID() {
    static char id;
    return &id;
  }

  Location loc;

  StmtCRTP(Location loc) : classID(_getClassID<Derived>()), loc(loc) {}
public:
  virtual const Location &getLoc() const override { return loc; }

  virtual ClassID getClassID() const override { return classID; }

  static bool classof(const StmtBase *stmt) {
    return stmt->getClassID() == _getClassID<Derived>();
  }

  virtual void print(std::ostream &os) const override {
    cast<Derived>(this)->printImpl(os);
  }

  virtual void accept(ASTVisitor &visitor) override {
    visitor.visit(static_cast<Derived&>(*this));
  }
};

// template<typename Derived>
class ScopedMixin {
  std::unique_ptr<Scope> scope;
public:
  Scope* getScope() const { return scope.get(); }

  void setScope(std::unique_ptr<Scope> newScope) {
    assert(scope == nullptr && "Scope has already been set");
    scope = std::move(newScope);
  }

  // static bool classof(const StmtBase* stmt) {
  //   // 派发给子类实现
  //   return Derived::classof(stmt);
  // }
};

class ExpressionStmt : public StmtCRTP<ExpressionStmt> {
private:
  std::unique_ptr<ExprBase> expression;

public:
  ExpressionStmt(std::unique_ptr< ExprBase> expression)
      : StmtCRTP<ExpressionStmt>(expression->getLoc()),
        expression(std::move(expression)) {}

  ExprBase *getExpression() const { return expression.get(); }

  void printImpl(std::ostream &os) const {
    expression->print(os);
    os << ";";
  }
};

// template<typename Derived>
class Declaration {
protected:
  std::unique_ptr<Symbol> symbol = nullptr;
public:
  virtual Symbol* getSymbol() const { return symbol ? symbol.get() : nullptr; }
  virtual void setSymbol(std::unique_ptr<Symbol> newSymbol) {
    assert(symbol == nullptr && "Symbol has already been set");
    symbol = std::move(newSymbol);
  }

  // static bool classof(const StmtBase* stmt) {
  //   // 派发给子类实现
  //   return Derived::classof(stmt);
  // }
};

class VarDeclStmt : public Declaration,
                   public StmtCRTP<VarDeclStmt> {
private:
  std::string name;
  std::unique_ptr<ExprBase> initializer = nullptr;

public:
  VarDeclStmt(const std::string &name, std::unique_ptr<ExprBase> initializer)
      : StmtCRTP<VarDeclStmt>(initializer->getLoc()), name(name),
        initializer(std::move(initializer)) {}
  VarDeclStmt(const std::string &name, Location loc)
      : StmtCRTP<VarDeclStmt>(std::move(loc)), name(name), initializer(nullptr) {}
  ~VarDeclStmt() override = default;

  const std::string &getName() const { return name; }

  ExprBase *getInitializer() const {
    return initializer ? initializer.get() : nullptr;
  }

  void printImpl(std::ostream &os) const {
    os << "var ";
    if (symbol) {
      symbol->print(os);
    }
    else {
      os << name;
    }

    if (initializer) {
      os << " = ";
      initializer->print(os);
    }
    os << ";";
  }
};

class BlockStmt : public ScopedMixin,
                  public StmtCRTP<BlockStmt> {
protected:
  std::vector<std::unique_ptr<StmtBase>> statements;
public:
  BlockStmt(std::vector<std::unique_ptr<StmtBase>> statements,
            Location location)
      : StmtCRTP<BlockStmt>(location), statements(std::move(statements)) {}
  ~BlockStmt() override = default;

  std::vector<std::unique_ptr<StmtBase>> &getStatements() { return statements; }

  void printImpl(std::ostream &os) const {
    os << "{" << std::endl;
    for (const auto &stmt : statements) {
      stmt->print(os);
      os << std::endl;
    }
    os << "}";
  }
};

class FunctionDeclStmt : public Declaration,
                          public ScopedMixin,
                          public StmtCRTP<FunctionDeclStmt> {
private:
  std::string name;
  std::vector<std::unique_ptr<VariableExpr>> parameters;
  std::unique_ptr<BlockStmt> body;

public:
  FunctionDeclStmt(std::string name,
               std::vector<std::unique_ptr<VariableExpr>> parameters,
               std::unique_ptr<BlockStmt> body)
      : StmtCRTP<FunctionDeclStmt>(body->getLoc()), name(std::move(name)),
        parameters(std::move(parameters)), body(std::move(body)) {}
  ~FunctionDeclStmt() override = default;

  const std::string &getName() const { return name; }

  std::vector<std::unique_ptr<VariableExpr>> &getParameters() {
    return parameters;
  }
  BlockStmt *getBody() const { return body.get(); }

  void printImpl(std::ostream &os) const {
    os << "fun " << name << "(";
    for (const auto &param : parameters) {
      param->print(os);
      os << ", ";
    }
    os << ") " << std::endl;
    body->print(os);
  }
};

class ClassDeclStmt : public Declaration,
                      public ScopedMixin,
                      public StmtCRTP<ClassDeclStmt> {
private:
  std::string className;
  std::optional<std::string> superclassName;
  std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields;
  std::unordered_map<std::string, std::unique_ptr<FunctionDeclStmt>> methods;

public:
  ClassDeclStmt(
      std::string name, std::optional<std::string> superclassName,
      std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields,
      std::unordered_map<std::string, std::unique_ptr<FunctionDeclStmt>> methods,
      Location loc)
      : StmtCRTP<ClassDeclStmt>(loc), className(name), superclassName(std::move(superclassName)),
        fields(std::move(fields)), methods(std::move(methods)) {}
  ClassDeclStmt(
      std::string name,
      std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields,
      std::unordered_map<std::string, std::unique_ptr<FunctionDeclStmt>> methods,
      Location loc)
      : ClassDeclStmt(std::move(name), std::nullopt, std::move(fields), std::move(methods) , loc) {}
  ~ClassDeclStmt() override = default;

  bool hasSuperclass() const { return superclassName.has_value(); }
  const std::string &getSuperclassName() const { return *superclassName; }
  const std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> &
  getFields() {
    return fields;
  }
  const std::unordered_map<std::string, std::unique_ptr<FunctionDeclStmt>> &
  getMethods() {
    return methods;
  }

  void printImpl(std::ostream &os) const {
    os << "class " << className;
    if (hasSuperclass()) {
      os << " < " << getSuperclassName();
    }
    os << " {" << std::endl;
    for (const auto &field : fields) {
      field.second->print(os);
      os << std::endl;
    }

    for (const auto &method : methods) {
      method.second->print(os);
      os << std::endl;
    }
    os << "}";
  }
};

class IfStmt : public ScopedMixin,
                public StmtCRTP<IfStmt> {
private:
  std::unique_ptr<ExprBase> condition;
  std::unique_ptr<BlockStmt> thenBlock;
  std::unique_ptr<BlockStmt> elseBlock;

public:
  IfStmt(std::unique_ptr<ExprBase> condition,
         std::unique_ptr<BlockStmt> thenBlock,
         std::unique_ptr<BlockStmt> elseBlock, Location loc)
      : StmtCRTP<IfStmt>(loc), condition(std::move(condition)),
        thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}
  ~IfStmt() override = default;

  ExprBase *getCondition() const { return condition.get(); }
  BlockStmt *getThenBranch() const { return thenBlock.get(); }
  BlockStmt *getElseBranch() const { return elseBlock.get(); }
  bool hasElseBranch() const { return elseBlock != nullptr; }

  void printImpl(std::ostream &os) const {
    os << "if (";
    condition->print(os);
    os << ") " << std::endl;
    thenBlock->print(os);
    if (elseBlock) {
      os << " else ";
      elseBlock->print(os);
    }
  }
};

class ReturnStmt : public StmtCRTP<ReturnStmt> {
private:
  std::unique_ptr<ExprBase> value;

public:
  ReturnStmt(std::unique_ptr<ExprBase> value, Location loc)
      : StmtCRTP<ReturnStmt>(loc), value(std::move(value)) {}
  ReturnStmt(Location loc)
      : StmtCRTP<ReturnStmt>(loc), value(nullptr) {}
  ~ReturnStmt() override = default;

  ExprBase *getValue() {
    return value.get();
  }

  void printImpl(std::ostream &os) const {
    os << "return";
    if (value) {
      os << " ";
      value->print(os);
    }
    os << ";";
  }
};

class ForStmt : public ScopedMixin,
                public StmtCRTP<ForStmt> {
protected:
  std::unique_ptr<StmtBase> initializer;
  std::unique_ptr<ExprBase> condition;
  std::unique_ptr<ExprBase> increment;
  std::unique_ptr<BlockStmt> body;

public:
  ForStmt(std::unique_ptr<StmtBase> initializer,
          std::unique_ptr<ExprBase> condition,
          std::unique_ptr<ExprBase> increment, std::unique_ptr<BlockStmt> body, Location loc)
      : StmtCRTP<ForStmt>(loc), initializer(std::move(initializer)),
        condition(std::move(condition)), increment(std::move(increment)),
        body(std::move(body)) {}
  ~ForStmt() override = default;

  StmtBase *getInitializer() const { return initializer.get(); }
  ExprBase *getCondition() const { return condition.get(); }
  ExprBase *getIncrement() const { return increment.get(); }
  BlockStmt *getBody() const { return body.get(); }

  void printImpl(std::ostream &os) const {
    os << "for (" << std::endl;
    if (initializer) {
      os << "initializer: ";
      initializer->print(os);
      os << std::endl;
    }
    if (condition) {
      os << "condition: ";
      condition->print(os);
      os << "; " << std::endl;
    }
    if (increment) {
      os << "increment: ";
      increment->print(os);
      os << std::endl;
    }
    os << ") ";
    body->print(os);
  }
};

class WhileStmt : public ScopedMixin,
                  public StmtCRTP<WhileStmt> {
private:
  std::unique_ptr<ExprBase> condition;
  std::unique_ptr<BlockStmt> body;
public:
  WhileStmt(std::unique_ptr<ExprBase> condition,
            std::unique_ptr<BlockStmt> body, Location loc)
      : StmtCRTP<WhileStmt>(loc), condition(std::move(condition)),
        body(std::move(body)) {}
  ~WhileStmt() override = default;

  void printImpl(std::ostream &os) const {
    os << "while (";
    condition->print(os);
    os << std::endl << ") ";
    body->print(os);
  }
};

class BreakStmt : public StmtCRTP<BreakStmt> {
public:
  BreakStmt(Location loc) : StmtCRTP<BreakStmt>(loc) {}
  ~BreakStmt() override = default;

  void printImpl(std::ostream &os) const {
    os << "break;";
  }
};

class ContinueStmt : public StmtCRTP<ContinueStmt> {
public:
  ContinueStmt(Location loc) : StmtCRTP<ContinueStmt>(loc) {}
  ~ContinueStmt() override = default;

  void printImpl(std::ostream &os) const {
    os << "continue;";
  }
};
} // namespace lox

#endif // STMT_H