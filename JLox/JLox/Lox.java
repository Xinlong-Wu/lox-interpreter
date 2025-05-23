package JLox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

import JLox.Expression.Stmt;
import JLox.Interpreter.Interpreter;
import JLox.Interpreter.Resolver;
import JLox.Token.Token;
import JLox.Token.TokenType;
import JLox.Parser.Parser;
import JLox.Error.RuntimeError;

public class Lox {
    private static final Interpreter interpreter = new Interpreter();
    static boolean hadError = false;
    static boolean hadRuntimeError = false;
    static boolean suppressError = true;

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64);
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        run(new String(bytes, Charset.defaultCharset()));
        // Indicate an error in the exit code.
        if (hadError)
            System.exit(65);
        if (hadRuntimeError)
            System.exit(70);
    }

    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);
        suppressError = false;
        String lines = null;
        for (;;) {
            if (lines == null){
                System.out.print("> ");
                lines = reader.readLine();
            }
            else
                lines += reader.readLine();
            if (lines == null)
                break;
            run(lines);
            if (!hadError)
                lines = null;
            hadError = false;
        }
    }

    private static void run(String source) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();

        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();

        // Stop if there was a syntax error.
        if (hadError) return;

        Resolver resolver = new Resolver(interpreter);
        resolver.resolve(statements);

        // Stop if there was a resolution error.
        if (hadError) return;

        interpreter.interpret(statements);
        // System.out.println(new AstPrinter().print(expression));
    }

    static void error(int line, int column, String message) {
        report(line, column, "", message);
    }

    private static void report(int line, int column, String where,
            String message) {
        System.err.println(
                "[line " + line + ":" + column + "] Error" + where + ": " + message);
    }

    public static void error(Token token, String message) {
        if (suppressError)
            if (token.type == TokenType.EOF) {
                report(token.line, token.column, " at end", message);
            } else {
                report(token.line, token.column, " at '" + token.lexeme + "'", message);
            }
        hadError = true;
    }

    public static void warning(Token token, String message) {
        if (token.type == TokenType.EOF) {
            report(token.line, token.column, " at end", message);
        } else {
            report(token.line, token.column, " at '" + token.lexeme + "'", message);
        }
    }

    public static void runtimeError(RuntimeError error) {
        System.err.println(error.getMessage() +
                "\n[line " + error.token.line + ":" + error.token.column + "]");
        hadRuntimeError = true;
    }
}
