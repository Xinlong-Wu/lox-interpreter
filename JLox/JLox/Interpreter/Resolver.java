package JLox.Interpreter;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;

import JLox.Lox;
import JLox.Expression.Expr;
import JLox.Expression.Stmt;
import JLox.Token.Token;

import static JLox.Token.TokenType.*;

public class Resolver implements Expr.Visitor<Void>, Stmt.Visitor<Void> {
  private final Interpreter interpreter;
  private final Stack<Map<Token, Integer>> scopes = new Stack<>();
  private FunctionType currentFunction = FunctionType.NONE;
  private boolean inLoop = false;

  public Resolver(Interpreter interpreter) {
    this.interpreter = interpreter;
  }

  private static class variableState {
    static Integer DECLARED = -1;
    static Integer DEFINED = 0;
  }

  private enum FunctionType {
    NONE,
    FUNCTION,
    METHOD,
    INITIALIZER,
  }

  private enum ClassType {
    NONE,
    CLASS
  }

  private ClassType currentClass = ClassType.NONE;

  @Override
  public Void visitBlockStmt(Stmt.Block stmt) {
    beginScope();
    resolve(stmt.statements);
    endScope();
    return null;
  }

  @Override
  public Void visitClassStmt(Stmt.Class stmt) {
    ClassType enclosingClass = currentClass;
    currentClass = ClassType.CLASS;
    declare(stmt.name);
    define(stmt.name);
    beginScope();
    define(new Token(THIS, "this", null, 0, 0));
    for (Stmt.Function method : stmt.methods) {
      FunctionType declaration = FunctionType.METHOD;
      if (method.name.lexeme.equals("init")) {
        declaration = FunctionType.INITIALIZER;
      }
      resolveFunction(method, declaration);
    }
    endScope();
    currentClass = enclosingClass;
    return null;
  }

  @Override
  public Void visitVarStmt(Stmt.Var stmt) {
    declare(stmt.name);
    if (stmt.initializer != null) {
      resolve(stmt.initializer);
    }
    define(stmt.name);
    return null;
  }

  @Override
  public Void visitVariableExpr(Expr.Variable expr) {
    if (!scopes.isEmpty() &&
        scopes.peek().get(expr.name) == variableState.DECLARED) {
      Lox.error(expr.name,
          "Can't read local variable in its own initializer.");
    }

    resolveLocal(expr, expr.name);
    return null;
  }

  @Override
  public Void visitAssignExpr(Expr.Assign expr) {
    resolve(expr.value);
    resolveLocal(expr, expr.name);
    return null;
  }

  @Override
  public Void visitBinaryExpr(Expr.Binary expr) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitCallExpr(Expr.Call expr) {
    resolve(expr.callee);

    for (Expr argument : expr.arguments) {
      resolve(argument);
    }

    return null;
  }

  @Override
  public Void visitGetExpr(Expr.Get expr) {
    resolve(expr.object);
    return null;
  }

  @Override
  public Void visitGroupingExpr(Expr.Grouping expr) {
    resolve(expr.expression);
    return null;
  }

  @Override
  public Void visitLiteralExpr(Expr.Literal expr) {
    return null;
  }

  @Override
  public Void visitLogicalExpr(Expr.Logical expr) {
    resolve(expr.left);
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitSetExpr(Expr.Set expr) {
    resolve(expr.value);
    resolve(expr.object);
    return null;
  }

  @Override
  public Void visitThisExpr(Expr.This expr) {
    if (currentClass == ClassType.NONE) {
      Lox.error(expr.keyword,
          "Can't use 'this' outside of a class.");
      return null;
    }

    resolveLocal(expr, expr.keyword);
    return null;
  }

  @Override
  public Void visitUnaryExpr(Expr.Unary expr) {
    resolve(expr.right);
    return null;
  }

  @Override
  public Void visitBreakStmt(Stmt.Break stmt) {
    if (!inLoop)
      Lox.error(stmt.keyword, "Cannot use 'break' outside of a loop.");
    return null;
  }

  @Override
  public Void visitContinueStmt(Stmt.Continue stmt) {
    if (!inLoop)
      Lox.error(stmt.keyword, "Cannot use 'continue' outside of a loop.");
    return null;
  }

  @Override
  public Void visitLambdaExpr(Expr.Lambda expr) {
    resolveFunction(expr.func, FunctionType.FUNCTION);
    return null;
  }

  @Override
  public Void visitFunctionStmt(Stmt.Function stmt) {
    declare(stmt.name);
    define(stmt.name);

    resolveFunction(stmt, FunctionType.FUNCTION);
    return null;
  }

  @Override
  public Void visitExpressionStmt(Stmt.Expression stmt) {
    resolve(stmt.expression);
    return null;
  }

  @Override
  public Void visitIfStmt(Stmt.If stmt) {
    resolve(stmt.condition);
    resolve(stmt.thenBranch);
    if (stmt.elseBranch != null)
      resolve(stmt.elseBranch);
    return null;
  }

  @Override
  public Void visitPrintStmt(Stmt.Print stmt) {
    resolve(stmt.expression);
    return null;
  }

  @Override
  public Void visitReturnStmt(Stmt.Return stmt) {
    if (stmt.value != null) {
      if (currentFunction == FunctionType.INITIALIZER) {
        Lox.error(stmt.keyword,
            "Can't return a value from an initializer.");
      }
      resolve(stmt.value);
    }

    return null;
  }

  @Override
  public Void visitWhileStmt(Stmt.While stmt) {
    resolve(stmt.condition);
    boolean prevInLoop = inLoop;
    inLoop = true;
    resolve(stmt.body);
    inLoop = prevInLoop;
    return null;
  }

  public void resolve(List<Stmt> statements) {
    for (Stmt statement : statements) {
      resolve(statement);
    }
  }

  private void resolve(Stmt stmt) {
    stmt.accept(this);
  }

  private void resolve(Expr expr) {
    expr.accept(this);
  }

  private void resolveFunction(
      Stmt.Function function, FunctionType type) {
    FunctionType enclosingFunction = currentFunction;
    currentFunction = type;
    boolean prevInLoop = inLoop;
    inLoop = false;

    beginScope();
    for (Token param : function.params) {
      declare(param);
      define(param);
    }
    resolve(function.body);
    endScope();

    inLoop = prevInLoop;
    currentFunction = enclosingFunction;
  }

  private void beginScope() {
    scopes.push(new HashMap<Token, Integer>());
  }

  private void endScope() {
    for (Map.Entry<Token, Integer> entry : scopes.peek().entrySet()) {
      if (entry.getKey().type == THIS)
        continue;
      if (entry.getValue() == variableState.DEFINED) {
        Lox.warning(entry.getKey(),
            "Local variable is defined but never used.");
      }
    }
    scopes.pop();
  }

  private void declare(Token name) {
    if (scopes.isEmpty())
      return;

    Map<Token, Integer> scope = scopes.peek();
    if (scope.containsKey(name)) {
      Lox.error(name,
          "Already a variable with this name in this scope.");
    }
    scope.put(name, variableState.DECLARED);
  }

  private void define(Token name) {
    if (scopes.isEmpty())
      return;
    scopes.peek().put(name, variableState.DEFINED);
  }

  private void resolveLocal(Expr expr, Token name) {
    for (int i = scopes.size() - 1; i >= 0; i--) {
      HashMap<Token, Integer> scope = (HashMap<Token, Integer>) scopes.get(i);
      if (scope.containsKey(name)) {
        Integer state = scope.get(name);
        if (state == variableState.DECLARED)
          Lox.error(name, "Use of uninitialized variable");

        scope.put(name, state + 1);
        interpreter.resolve(expr, scopes.size() - 1 - i);
        return;
      }
    }
  }
}