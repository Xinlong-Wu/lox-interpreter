package JLox.Utils;

import JLox.Error.RuntimeError;
import JLox.Expression.Expr;
import JLox.Expression.Stmt;
import JLox.Expression.Stmt.*;
import JLox.Expression.Stmt.Class;
import JLox.Interpreter.LoxInstance;
import JLox.Expression.Expr.*;
import JLox.Token.Token;
import JLox.Token.TokenType;

public class AstPrinter implements Expr.Visitor<String>, Stmt.Visitor<String> {

    private static AstPrinter instance = new AstPrinter();

    public static AstPrinter getInstance() {
        return instance;
    }

    public String print(Expr expr) {
        return expr.accept(this);
    }

    public String print(Stmt stmt) {
        return stmt.accept(this);
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
        builder.append(expr.name.lexeme);
        return builder.toString();
    }

    @Override
    public String visitLogicalExpr(Logical expr) {
        StringBuilder builder = new StringBuilder();
        builder.append(expr.left.accept(this));
        builder.append(" ");
        builder.append(expr.operator.lexeme);
        builder.append(" ");
        builder.append(expr.right.accept(this));
        return builder.toString();
    }

    @Override
    public String visitBlockStmt(Block stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("{\n");
        for (Stmt statement : stmt.statements) {
            builder.append(statement.accept(this));
            builder.append("\n");
        }
        builder.append("}");
        return builder.toString();
    }

    @Override
    public String visitExpressionStmt(Expression stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append(stmt.expression.accept(this)).append(";");
        return builder.toString();
    }

    @Override
    public String visitIfStmt(If stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("if (").append(stmt.condition.accept(this)).append(") ");
        builder.append(stmt.thenBranch.accept(this));
        if (stmt.elseBranch != null) {
            builder.append(" else ").append(stmt.elseBranch.accept(this));
        }
        return builder.toString();
    }

    @Override
    public String visitPrintStmt(Print stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("print(").append(stmt.expression.accept(this)).append(");");
        return builder.toString();
    }

    @Override
    public String visitVarStmt(Var stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("var ").append(stmt.name.lexeme);
        if (stmt.initializer != null) {
            builder.append(" = ").append(stmt.initializer.accept(this));
        }
        return builder.toString();
    }

    @Override
    public String visitWhileStmt(While stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("while (").append(stmt.condition.accept(this)).append(") ");
        builder.append(stmt.body.accept(this));
        return builder.toString();
    }

    @Override
    public String visitBreakStmt(Break stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("break;");
        return builder.toString();
    }

    @Override
    public String visitContinueStmt(Continue stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("continue;");
        return builder.toString();
    }

    @Override
    public String visitFunctionStmt(Function stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("fun ").append(stmt.name.lexeme).append("(");
        for (int i = 0; i < stmt.params.size(); i++) {
            if (i > 0)
                builder.append(", ");
            builder.append(stmt.params.get(i).lexeme);
        }
        builder.append(") ");
        builder.append("{\n");
        for (Stmt statement : stmt.body) {
            builder.append(statement.accept(this));
            builder.append("\n");
        }
        builder.append("}\n");
        return builder.toString();
    }

    @Override
    public String visitClassStmt(Class stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("class ").append(stmt.name.lexeme).append(" {\n");
        for (Stmt.Function method : stmt.methods) {
            builder.append(method.accept(this));
            builder.append("\n");
        }
        builder.append("}");
        return builder.toString();
    }

    @Override
    public String visitLambdaExpr(Lambda expr) {
        StringBuilder builder = new StringBuilder();
        builder.append("fun ").append("(");
        for (int i = 0; i < expr.func.params.size(); i++) {
            if (i > 0)
                builder.append(", ");
            builder.append(expr.func.params.get(i).lexeme);
        }
        builder.append(") ");
        builder.append("{\n");
        for (Stmt statement : expr.func.body) {
            builder.append(statement.accept(this));
            builder.append("\n");
        }
        builder.append("}");
        return builder.toString();
    }

    @Override
    public String visitCallExpr(Call expr) {
        StringBuilder builder = new StringBuilder();
        builder.append(expr.callee.accept(this)).append("(");
        for (int i = 0; i < expr.arguments.size(); i++) {
            if (i > 0)
                builder.append(", ");
            builder.append(expr.arguments.get(i).accept(this));
        }
        builder.append(")");
        return builder.toString();
    }

    @Override
    public String visitGetExpr(Expr.Get expr) {
        StringBuilder builder = new StringBuilder();
        builder.append(expr.object.accept(this)).append(".").append(expr.name.lexeme);
        return builder.toString();
    }

    @Override
    public String visitReturnStmt(Return stmt) {
        StringBuilder builder = new StringBuilder();
        builder.append("return");
        if (stmt.value != null) {
            builder.append(" ").append(stmt.value.accept(this));
        }
        builder.append(";");
        return builder.toString();
    }

    @Override
    public String visitSetExpr(Set expr) {
        StringBuilder builder = new StringBuilder();
        builder.append(expr.object.accept(this)).append(".").append(expr.name.lexeme).append(" = ");
        builder.append(expr.value.accept(this));
        return builder.toString();
    }

    @Override
    public String visitThisExpr(This expr) {
        StringBuilder builder = new StringBuilder();
        builder.append("this");
        return builder.toString();
    }
}
