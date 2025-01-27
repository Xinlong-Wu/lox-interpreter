package JLox.Utils;

import JLox.Expression.Expr;
import JLox.Expression.Expr.Assign;
import JLox.Expression.Expr.Variable;
import JLox.Token.Token;
import JLox.Token.TokenType;

public class AstPrinter implements Expr.Visitor<String> {
    public String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return parenthesize(expr.operator.lexeme,
                expr.left, expr.right);
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return parenthesize("group", expr.expression);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null)
            return "nil";
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return parenthesize(expr.operator.lexeme, expr.right);
    }

    @Override
    public String visitTernaryExpr(Expr.Ternary expr) {
        return parenthesize("?", expr.condition, expr.thenBranch, expr.elseBranch);
    }

    private String parenthesize(String name, Expr... exprs) {
        StringBuilder builder = new StringBuilder();

        builder.append("(").append(name);
        for (Expr expr : exprs) {
            builder.append(" ");
            builder.append(expr.accept(this));
        }
        builder.append(")");

        return builder.toString();
    }

    public static void main(String[] args) {
    Expr expression = new Expr.Binary(
        new Expr.Unary(
            new Token(TokenType.MINUS, "-", null, 1, 0),
            new Expr.Literal(123)),
        new Token(TokenType.STAR, "*", null, 1, 0),
        new Expr.Grouping(
            new Expr.Literal(45.67)));

    System.out.println(new AstPrinter().print(expression));
  }

    @Override
    public String visitAssignExpr(Assign expr) {
        StringBuilder builder = new StringBuilder();
        builder.append(expr.name.lexeme).append(" = ");
        builder.append(expr.value.accept(this));
        return builder.toString();
    }

    @Override
    public String visitVariableExpr(Variable expr) {
        StringBuilder builder = new StringBuilder();
        builder.append("var ");
        builder.append(expr.name.lexeme);
        return builder.toString();
    }
}
