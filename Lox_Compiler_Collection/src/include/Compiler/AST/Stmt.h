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

namespace lox {
class StmtBase : public ASTNode {
protected:
  Location loc;

  enum class Kind {
    ExpressionStmt,
    DeclarationStmt,
    VarDeclStmt,
    BlockStmt,
    ClassDeclStmt,
    FunctionDecl,
    IfStmt,
    WhileStmt,
    ForStmt,
    ReturnStmt
  };

public:
  StmtBase(Location location) : loc(location){};
  virtual ~StmtBase() = default;

  virtual const Location &getLoc() const { return loc; };

  virtual void print(std::ostream &os) const = 0;
  virtual void dump() const {
    this->print(std::cout);
    std::cout << std::endl;
  }

  virtual Kind getKind() const = 0;
  virtual void accept(ASTVisitor &visitor) = 0;
};

class ExpressionStmt : public StmtBase {
private:
  std::unique_ptr<ExprBase> expression;

public:
  ExpressionStmt(std::unique_ptr<ExprBase> expression)
      : StmtBase(expression->getLoc().getNextColumn()),
        expression(std::move(expression)) {}

  ExprBase *getExpression() const { return expression.get(); }

  virtual void print(std::ostream &os) const override {
    expression->print(os);
    os << ";";
  }

  TYPEID_SYSTEM(StmtBase, ExpressionStmt);
  ACCEPT_DECL();
};

class DeclarationStmt : public StmtBase {
protected:
  std::shared_ptr<Symbol> symbol;

public:
  DeclarationStmt(std::shared_ptr<Symbol> symbol, Location loc)
      : StmtBase(std::move(loc)), symbol(std::move(symbol)) {}
  DeclarationStmt(std::string name, Location loc)
      : DeclarationStmt(std::make_shared<Symbol>(name), std::move(loc)) {}
  DeclarationStmt(std::string name, std::shared_ptr<Type> type, Location loc)
      : DeclarationStmt(std::make_shared<Symbol>(name, type), std::move(loc)) {}
  ~DeclarationStmt() override = default;

  virtual const std::string &getName() const { return symbol->getName(); }
  virtual const std::shared_ptr<Type> &getType() const {
    return symbol->getType();
  }
  virtual std::shared_ptr<Symbol> &getSymbol() { return symbol; }
  virtual void setSymbol(std::shared_ptr<Symbol> symbol) {
    this->symbol = std::move(symbol);
  }

  TYPEID_SYSTEM(StmtBase, DeclarationStmt);
  ACCEPT_DECL();
};

class VarDeclStmt : public DeclarationStmt {
private:
  std::unique_ptr<ExprBase> initializer = nullptr;

public:
  VarDeclStmt(const std::string &name, std::unique_ptr<ExprBase> initializer)
      : VarDeclStmt(std::make_shared<Symbol>(name), std::move(initializer)) {}
  VarDeclStmt(const std::string &name, Location loc)
      : VarDeclStmt(std::make_shared<Symbol>(name), std::move(loc)) {}
  VarDeclStmt(std::shared_ptr<Symbol> symbol,
              std::unique_ptr<ExprBase> initializer)
      : DeclarationStmt(std::move(symbol), initializer->getLoc()),
        initializer(std::move(initializer)) {}
  VarDeclStmt(std::shared_ptr<Symbol> symbol, Location loc)
      : DeclarationStmt(std::move(symbol), std::move(loc)) {}
  ~VarDeclStmt() override = default;

  ExprBase *getInitializer() const {
    return initializer ? initializer.get() : nullptr;
  }
  void setInitializer(std::unique_ptr<ExprBase> expr) {
    initializer = std::move(expr);
  }

  virtual void print(std::ostream &os) const override {
    os << "var ";
    symbol->print(os);
    if (initializer) {
      os << " = ";
      initializer->print(os);
    }
    os << ";";
  }

  TYPEID_SYSTEM(StmtBase, VarDeclStmt);
  ACCEPT_DECL();
};

class BlockStmt : public StmtBase {
public:
  std::vector<std::unique_ptr<StmtBase>> statements;

  BlockStmt(std::vector<std::unique_ptr<StmtBase>> statements,
            Location location)
      : StmtBase(location), statements(std::move(statements)) {}
  BlockStmt(std::vector<std::unique_ptr<StmtBase>> statements)
      : BlockStmt(std::move(statements),
                  statements[statements.size() - 1]->getLoc().getNextColumn()) {
  }
  ~BlockStmt() override = default;

  std::vector<std::unique_ptr<StmtBase>> &getStatements() { return statements; }

  virtual void print(std::ostream &os) const override {
    os << "BlockStmt: " << "{" << std::endl;
    for (const auto &stmt : statements) {
      stmt->print(os);
      os << std::endl;
    }
    os << "}" << std::endl;
  }

  TYPEID_SYSTEM(StmtBase, BlockStmt);
  ACCEPT_DECL();
};

class FunctionDecl : public DeclarationStmt {
private:
  std::vector<std::unique_ptr<VariableExpr>> parameters;
  std::unique_ptr<BlockStmt> body;

public:
  FunctionDecl(std::string name,
               std::vector<std::unique_ptr<VariableExpr>> parameters,
               std::unique_ptr<BlockStmt> body)
      : DeclarationStmt(std::make_shared<Symbol>(name), body->getLoc()),
        parameters(std::move(parameters)), body(std::move(body)) {}
  ~FunctionDecl() override = default;

  std::vector<std::unique_ptr<VariableExpr>> &getParameters() {
    return parameters;
  }
  BlockStmt *getBody() const { return body.get(); }

  virtual void print(std::ostream &os) const override {
    os << "function " << getName() << "(";
    for (const auto &param : parameters) {
      param->print(os);
      os << ", ";
    }
    os << ") " << std::endl;
    body->print(os);
    os << std::endl;
  }

  TYPEID_SYSTEM(StmtBase, FunctionDecl);
  ACCEPT_DECL();
};

class ClassDeclStmt : public DeclarationStmt {
private:
  std::optional<std::string> superclass;
  std::unordered_map<std::string, std::unique_ptr<DeclarationStmt>> fields;

  // for semantic analysis, we need to keep track of the class scope
  std::shared_ptr<Scope> classScope;

public:
  ClassDeclStmt(
      std::string name, std::optional<std::string> superclass,
      std::unordered_map<std::string, std::unique_ptr<DeclarationStmt>> fields,
      Location loc)
      : DeclarationStmt(std::move(name), loc),
        superclass(std::move(superclass)), fields(std::move(fields)) {}
  ClassDeclStmt(
      std::string name,
      std::unordered_map<std::string, std::unique_ptr<DeclarationStmt>> fields,
      Location loc)
      : ClassDeclStmt(std::move(name), std::nullopt, std::move(fields), loc) {}
  ~ClassDeclStmt() override = default;

  bool hasSuperclass() const { return superclass.has_value(); }
  const std::string &getSuperclassName() const { return *superclass; }
  std::unordered_map<std::string, std::unique_ptr<DeclarationStmt>> &
  getFields() {
    return fields;
  }
  std::shared_ptr<Scope> &getClassScope() { return classScope; }
  void setClassScope(std::shared_ptr<Scope> scope) {
    assert(classScope == nullptr && "Class scope has already been set");
    classScope = std::move(scope);
  }

  virtual void print(std::ostream &os) const override {
    os << "class " << getName();
    if (superclass) {
      os << " < " << *superclass;
    }
    os << " {" << std::endl;
    for (const auto &field : fields) {
      field.second->print(os);
      os << std::endl;
    }
    os << "}" << std::endl;
  }

  TYPEID_SYSTEM(StmtBase, ClassDeclStmt);
  ACCEPT_DECL();
};

class IfStmt : public StmtBase {
private:
  std::unique_ptr<ExprBase> condition;
  std::unique_ptr<BlockStmt> thenBlock;
  std::unique_ptr<BlockStmt> elseBlock;

public:
  IfStmt(std::unique_ptr<ExprBase> condition,
         std::unique_ptr<BlockStmt> thenBlock,
         std::unique_ptr<BlockStmt> elseBlock, Location location)
      : StmtBase(location), condition(std::move(condition)),
        thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}
  ~IfStmt() override = default;

  ExprBase *getCondition() const { return condition.get(); }
  BlockStmt *getThenBranch() const { return thenBlock.get(); }
  BlockStmt *getElseBranch() const { return elseBlock.get(); }
  bool hasElseBranch() const { return elseBlock != nullptr; }

  virtual void print(std::ostream &os) const override {
    os << "if (";
    condition->print(os);
    os << ") " << std::endl;
    thenBlock->print(os);
    if (elseBlock) {
      os << " else ";
      elseBlock->print(os);
    }
  }

  TYPEID_SYSTEM(StmtBase, IfStmt);
  ACCEPT_DECL();
};

class ReturnStmt : public StmtBase {
private:
  std::unique_ptr<ExprBase> value;

public:
  ReturnStmt(std::unique_ptr<ExprBase> value)
      : StmtBase(value->getLoc()), value(std::move(value)) {}
  ReturnStmt(Location loc) : StmtBase(loc) {}
  ~ReturnStmt() override = default;

  ExprBase *getValue() {
    return value.get();
  }

  virtual void print(std::ostream &os) const override {
    os << "return ";
    if (value) {
      value->print(os);
    }
    os << ";";
  }

  TYPEID_SYSTEM(StmtBase, ReturnStmt);
  ACCEPT_DECL();
};

class ForStmt : public StmtBase {
protected:
  std::unique_ptr<StmtBase> initializer;
  std::unique_ptr<ExprBase> condition;
  std::unique_ptr<ExprBase> increment;
  std::unique_ptr<BlockStmt> body;

public:
  ForStmt(std::unique_ptr<StmtBase> initializer,
          std::unique_ptr<ExprBase> condition,
          std::unique_ptr<ExprBase> increment, std::unique_ptr<BlockStmt> body)
      : StmtBase(body->getLoc()), initializer(std::move(initializer)),
        condition(std::move(condition)), increment(std::move(increment)),
        body(std::move(body)) {}
  ~ForStmt() override = default;

  virtual void print(std::ostream &os) const override {
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

  TYPEID_SYSTEM(StmtBase, ForStmt);
  ACCEPT_DECL();
};

class WhileStmt : public ForStmt {
public:
  WhileStmt(std::unique_ptr<ExprBase> condition,
            std::unique_ptr<BlockStmt> body)
      : ForStmt(nullptr, std::move(condition), nullptr, std::move(body)) {}
  ~WhileStmt() override = default;

  virtual void print(std::ostream &os) const override {
    os << "while (" << std::endl;
    os << "condition: ";
    condition->print(os);
    os << std::endl << ") ";
    body->print(os);
  }

  TYPEID_SYSTEM(StmtBase, WhileStmt);
  ACCEPT_DECL();
};
} // namespace lox

#endif // STMT_H