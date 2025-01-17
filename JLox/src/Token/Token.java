package Token;

public class Token {
    public final TokenType type;
    public final String lexeme;
    public final Object literal;
    final int line;
    final int column;

    public Token(TokenType type, String lexeme, Object literal, int line, int column) {
        this.type = type;
        this.lexeme = lexeme;
        this.literal = literal;
        this.line = line;
        this.column = column;
    }

    public String toString() {
        return type + " " + lexeme + " " + literal;
    }
}
