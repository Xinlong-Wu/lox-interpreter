package JLox.Expression;

import java.util.List;
import JLox.Token.Token;
import JLox.Utils.AstPrinter;

public abstract class Expr {
  public interface Visitor<R> {
    R visitAssignExpr(Assign expr);
    R visitBinaryExpr(Binary expr);
    R visitCallExpr(Call expr);
    R visitGetExpr(Get expr);
    R visitGroupingExpr(Grouping expr);
    R visitLiteralExpr(Literal expr);
    R visitLogicalExpr(Logical expr);
    R visitSetExpr(Set expr);
    R visitSuperExpr(Super expr);
    R visitThisExpr(This expr);
    R visitUnaryExpr(Unary expr);
    R visitLambdaExpr(Lambda expr);
    R visitVariableExpr(Variable expr);
  }
  public static class Assign extends Expr {
    public Assign(Token name, Expr value) {
      this.name = name;
      this.value = value;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitAssignExpr(this);
    }

    public final Token name;
    public final Expr value;
  }
  public static class Binary extends Expr {
    public Binary(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitBinaryExpr(this);
    }

    public final Expr left;
    public final Token operator;
    public final Expr right;
  }
  public static class Call extends Expr {
    public Call(Expr callee, Token paren, List<Expr> arguments) {
      this.callee = callee;
      this.paren = paren;
      this.arguments = arguments;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitCallExpr(this);
    }

    public final Expr callee;
    public final Token paren;
    public final List<Expr> arguments;
  }
  public static class Get extends Expr {
    public Get(Expr object, Token name) {
      this.object = object;
      this.name = name;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitGetExpr(this);
    }

    public final Expr object;
    public final Token name;
  }
  public static class Grouping extends Expr {
    public Grouping(Expr expression) {
      this.expression = expression;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitGroupingExpr(this);
    }

    public final Expr expression;
  }
  public static class Literal extends Expr {
    public Literal(Object value) {
      this.value = value;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLiteralExpr(this);
    }

    public final Object value;
  }
  public static class Logical extends Expr {
    public Logical(Expr left, Token operator, Expr right) {
      this.left = left;
      this.operator = operator;
      this.right = right;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLogicalExpr(this);
    }

    public final Expr left;
    public final Token operator;
    public final Expr right;
  }
  public static class Set extends Expr {
    public Set(Expr object, Token name, Expr value) {
      this.object = object;
      this.name = name;
      this.value = value;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitSetExpr(this);
    }

    public final Expr object;
    public final Token name;
    public final Expr value;
  }
  public static class Super extends Expr {
    public Super(Token keyword, Token method) {
      this.keyword = keyword;
      this.method = method;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitSuperExpr(this);
    }

    public final Token keyword;
    public final Token method;
  }
  public static class This extends Expr {
    public This(Token keyword) {
      this.keyword = keyword;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitThisExpr(this);
    }

    public final Token keyword;
  }
  public static class Unary extends Expr {
    public Unary(Token operator, Expr right) {
      this.operator = operator;
      this.right = right;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitUnaryExpr(this);
    }

    public final Token operator;
    public final Expr right;
  }
  public static class Lambda extends Expr {
    public Lambda(Stmt.Function func) {
      this.func = func;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitLambdaExpr(this);
    }

    public final Stmt.Function func;
  }
  public static class Variable extends Expr {
    public Variable(Token name) {
      this.name = name;
    }

    @Override
    public <R> R accept(Visitor<R> visitor) {
      return visitor.visitVariableExpr(this);
    }

    public final Token name;
  }

  public abstract <R> R accept(Visitor<R> visitor);

  public void dump() {
    System.out.println(AstPrinter.getInstance().print(this));
  }
}
