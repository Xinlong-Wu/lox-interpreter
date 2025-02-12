package JLox;

import static JLox.Token.TokenType.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import JLox.Token.Token;
import JLox.Token.TokenType; 

public class Scanner {
  private static final Map<String, TokenType> keywords;
  static {
    keywords = new HashMap<>();
    keywords.put("and",    AND);
    keywords.put("class",  CLASS);
    keywords.put("else",   ELSE);
    keywords.put("false",  FALSE);
    keywords.put("for",    FOR);
    keywords.put("fun",    FUN);
    keywords.put("if",     IF);
    keywords.put("nil",    NIL);
    keywords.put("or",     OR);
    keywords.put("print",  PRINT);
    keywords.put("return", RETURN);
    keywords.put("super",  SUPER);
    keywords.put("this",   THIS);
    keywords.put("true",   TRUE);
    keywords.put("var",    VAR);
    keywords.put("while",  WHILE);
    keywords.put("break",  BREAK);
    keywords.put("continue", CONTINUE);
    keywords.put("static", STATIC);
  }

  private final String source;
  private final List<Token> tokens = new ArrayList<>();
  private int start = 0;
  private int current = 0;
  private int line = 1;
  private int column = 0;

  Scanner(String source) {
    this.source = source;
  }

  List<Token> scanTokens() {
    while (!isAtEnd()) {
      // We are at the beginning of the next lexeme.
      start = current;
      scanToken();
    }

    tokens.add(new Token(EOF, "", null, line, column));
    return tokens;
  }

  private void scanToken() {
    char c = advance();
    switch (c) {
      case '(': addToken(LEFT_PAREN); break;
      case ')': addToken(RIGHT_PAREN); break;
      case '{': addToken(LEFT_BRACE); break;
      case '}': addToken(RIGHT_BRACE); break;
      case ',': addToken(COMMA); break;
      case '.': addToken(DOT); break;
      case '-': addToken(MINUS); break;
      case '+': addToken(PLUS); break;
      case ';': addToken(SEMICOLON); break;
      case '*': addToken(STAR); break; 
      case '%': addToken(MOD); break;

      case '!':
        addToken(match('=') ? BANG_EQUAL : BANG);
        break;
      case '=':
        addToken(match('=') ? EQUAL_EQUAL : EQUAL);
        break;
      case '<':
        addToken(match('=') ? LESS_EQUAL : LESS);
        break;
      case '>':
        addToken(match('=') ? GREATER_EQUAL : GREATER);
        break;
      case '/':
        if (match('/')) {
          // A comment goes until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } 
        else if (match('*')){
          int nestDeeps = 1;
          while (!isAtEnd()){
            if (peek() == '/' && peekNext() == '*'){
              nestDeeps++;
            }
            else if (peek() == '*' && peekNext() == '/'){
              nestDeeps--;
              if (nestDeeps == 0){
                advance();
                advance();
                break;
              }
            }
            advance();
          }
          if (nestDeeps != 0){
            Lox.error(line, column, "Unterminated comment.");
          }
        }
        else {
          addToken(SLASH);
        }
        break;
      
      case ' ':
      case '\r':
      case '\t':
        // Ignore whitespace.
        break;

      case '"': string(); break;

      case '\n':
        setNewLine();
        break;

      default:
        if (isDigit(c)) {
          number();
        } else if (isAlpha(c)) {
          identifier();
        } else {
          Lox.error(line, column, "Unexpected character.");
        }
        break;
    }
  }

  private void identifier() {
    while (isAlphaNumeric(peek())) advance();

    String text = source.substring(start, current);
    TokenType type = keywords.get(text);
    if (type == null) type = IDENTIFIER;
    addToken(type);
  }

  private void number() {
    while (isDigit(peek())) advance();

    // Look for a fractional part.
    if (peek() == '.' && isDigit(peekNext())) {
      // Consume the "."
      advance();

      while (isDigit(peek())) advance();
    }

    addToken(NUMBER,
        Double.parseDouble(source.substring(start, current)));
  }

  private void string() {
    StringBuilder valueBuilder = new StringBuilder();
    while (peek() != '"' && !isAtEnd()) {
      if (peek() == '\n') setNewLine();
      if (peek() == '\\') advance();
      valueBuilder.append(advance());
    }

    if (isAtEnd()) {
      Lox.error(line, column, "Unterminated string.");
      return;
    }

    // The closing ".
    advance();

    addToken(STRING, valueBuilder.toString());
  }

  private void setNewLine() {
    line++;
    column = 0;
  }

  private boolean match(char expected) {
    if (isAtEnd()) return false;
    if (peek() != expected) return false;

    advance();
    return true;
  }

  private char peek() {
    if (isAtEnd()) return '\0';
    return source.charAt(current);
  }

  private char peekNext() {
    if (current + 1 >= source.length()) return '\0';
    return source.charAt(current + 1);
  } 

  private boolean isAlpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
            c == '_';
  }

  private boolean isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
  }

  private boolean isDigit(char c) {
    return c >= '0' && c <= '9';
  } 

  private boolean isAtEnd() {
    return current >= source.length();
  }

  private char advance() {
    column++;
    return source.charAt(current++);
  }

  private void addToken(TokenType type) {
    addToken(type, null);
  }

  private void addToken(TokenType type, Object literal) {
    String text = source.substring(start, current);
    tokens.add(new Token(type, text, literal, line, column));
  }
}
