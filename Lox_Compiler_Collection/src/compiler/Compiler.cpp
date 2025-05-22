#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstdlib>
#include <optional>
#include <unordered_map>

#include "_common.h"
#include "memory.h"
#include "disassembler/lineinfo.h"
#include "Compiler/Scanner/Token.h"
#include "Compiler/compiler.h"
#include "Compiler/Parser/Parser.h"
#include "vm/object.h"

#ifdef DEBUG_PRINT_CODE
#include "disassembler/debug.h"
#endif

using namespace lox;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  lox::Token name;
  int depth;
  bool isCaptured;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_METHOD,
  TYPE_INITIALIZER,
  TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  bool hasSuperclass;
} ClassCompiler;

Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

std::optional<lox::Parser> parser;

static Chunk* currentChunk() {
  return &current->function->chunk;
}

void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, createLineInfo(parser->getPreviousToken().getLoction().getLine(), parser->getPreviousToken().getLoction().getColumn()));
}

void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) parser->parseError("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  return currentChunk()->count - 2;
}

void emitWord(uint8_t byte1, uint8_t byte2, uint8_t byte3, uint8_t byte4) {
  emitBytes(byte1, byte2);
  emitBytes(byte3, byte4);
}

void emitReturn() {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }
  emitByte(OP_RETURN);
}

static ObjFunction* endCompiler() {
  emitReturn();
  ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
  if (debug && !parser->hasError()) {
    disassembleChunk(currentChunk(), function->name != NULL
        ? function->name->chars : "<script>");
  }
#endif
  current = current->enclosing;

return function;
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
    current->locals[current->localCount - 1].depth >
       current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static void expression();
static void statement();
static void declaration();
static int resolveLocal(Compiler* compiler, lox::Token& name);
static void and_(bool canAssign);
static void or_(bool canAssign);
static uint8_t argumentList();
static int resolveUpvalue(Compiler* compiler, lox::Token& name);
static bool identifiersEqual(lox::Token& a, lox::Token& b);
static uint64_t identifierConstant(lox::Token& name);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void binary(bool canAssign) {
  TokenType operatorType = parser->getPreviousToken().getType();
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case lox::TokenType::TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case lox::TokenType::TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case lox::TokenType::TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case lox::TokenType::TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case lox::TokenType::TOKEN_LESS:          emitByte(OP_LESS); break;
    case lox::TokenType::TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case lox::TokenType::TOKEN_PLUS:          emitByte(OP_ADD); break;
    case lox::TokenType::TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case lox::TokenType::TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case lox::TokenType::TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default: return; // Unreachable.
  }
}

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
  parser->parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect property name after '.'.");
  uint8_t name = identifierConstant(parser->getPreviousToken());

  if (canAssign && parser->parseOptional(lox::TokenType::TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else if (parser->parseOptional(lox::TokenType::TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

static void literal(bool canAssign) {
  switch (parser->getPreviousToken().getType()) {
    case lox::TokenType::TOKEN_FALSE: emitByte(OP_FALSE); break;
    case lox::TokenType::TOKEN_NIL: emitByte(OP_NIL); break;
    case lox::TokenType::TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}

static void grouping(bool canAssign) {
  expression();
  parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
}

uint64_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  return (uint64_t)constant;
}

void emitConstant(Value value) {
  uint64_t constant = makeConstant(value);
  if (constant > UINT8_MAX) {
    emitBytes(OP_CONSTANT, constant >> 16);
    emitBytes(constant >> 8, constant);
  }
  else {
    emitBytes(OP_CONSTANT, constant);
  }
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    parser->parseError("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->scopeDepth = 0;
  compiler->function = newFunction();
  current = compiler;
  if (type != TYPE_SCRIPT) {
    std::string_view str = parser->getPreviousToken().getTokenString();
    current->function->name = copyString(str.data(), str.length());
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  if (type != TYPE_FUNCTION) {
    local->name = lox::Token("this");
  } else {
    local->name = lox::Token("");
  }
}

static void number(bool canAssign) {
  double value = std::stod(std::string(parser->getPreviousToken().getTokenString()));
  emitConstant(NUMBER_VAL(value));
}

static void parseString(bool canAssign) {
  std::string_view str = parser->getPreviousToken().getTokenString();
  emitConstant(OBJ_VAL(copyString(str.data() + 1, str.length() - 2)));
}

static void namedVariable(lox::Token name, bool canAssign) {
  uint8_t op = OP_GET_GLOBAL;
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }
  if (canAssign && parser->parseOptional(lox::TokenType::TOKEN_EQUAL)) {
    expression();
    op = setOp;
  } else {
    op = getOp;
  }
  if (arg > UINT8_MAX) {
    emitWord(op + 1, arg >> 16, arg >> 8, arg);
  }
  else {
    emitBytes(op, arg);
  }
}

static void variable(bool canAssign) {
  namedVariable(parser->getPreviousToken(), canAssign);
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    parser->parseError("Can't use 'super' outside of a class.");
  } else if (!currentClass->hasSuperclass) {
    parser->parseError("Can't use 'super' in a class with no superclass.");
  }

  parser->parse(lox::TokenType::TOKEN_DOT);
  parser->parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(parser->getPreviousToken());

  namedVariable(lox::Token("this"), false);
  if (parser->parseOptional(lox::TokenType::TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(lox::Token("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(lox::Token("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    parser->parseError("Can't use 'this' outside of a class.");
    return;
  }

  variable(false);
}

static void unary(bool canAssign) {
  TokenType operatorType = parser->getPreviousToken().getType();

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case lox::TokenType::TOKEN_BANG: emitByte(OP_NOT); break;
    case lox::TokenType::TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

std::unordered_map<lox::TokenType, ParseRule> rules = {
  {lox::TokenType::TOKEN_LEFT_PAREN,    {grouping, call,   PREC_CALL}},
  {lox::TokenType::TOKEN_RIGHT_PAREN,   {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_LEFT_BRACE,    {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_RIGHT_BRACE,   {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_COMMA,         {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_DOT,           {NULL,     dot,    PREC_CALL}},
  {lox::TokenType::TOKEN_MINUS,         {unary,    binary, PREC_TERM}},
  {lox::TokenType::TOKEN_PLUS,          {NULL,     binary, PREC_TERM}},
  {lox::TokenType::TOKEN_SEMICOLON,     {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_SLASH,         {NULL,     binary, PREC_FACTOR}},
  {lox::TokenType::TOKEN_STAR,          {NULL,     binary, PREC_FACTOR}},
  {lox::TokenType::TOKEN_BANG,          {unary,    NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_BANG_EQUAL,    {NULL,     binary, PREC_EQUALITY}},
  {lox::TokenType::TOKEN_EQUAL_EQUAL,   {NULL,     binary, PREC_EQUALITY}},
  {lox::TokenType::TOKEN_GREATER,       {NULL,     binary, PREC_COMPARISON}},
  {lox::TokenType::TOKEN_GREATER_EQUAL, {NULL,     binary, PREC_COMPARISON}},
  {lox::TokenType::TOKEN_LESS,          {NULL,     binary, PREC_COMPARISON}},
  {lox::TokenType::TOKEN_LESS_EQUAL,    {NULL,     binary, PREC_COMPARISON}},
  {lox::TokenType::TOKEN_IDENTIFIER,    {variable, NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_STRING,        {parseString,   NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_NUMBER,        {number,   NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_AND,           {NULL,     and_,   PREC_AND}},
  {lox::TokenType::TOKEN_CLASS,         {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_ELSE,          {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_FALSE,         {literal,  NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_FOR,           {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_FUN,           {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_IF,            {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_NIL,           {literal,  NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_OR,            {NULL,     or_,    PREC_OR}},
  {lox::TokenType::TOKEN_PRINT,         {call,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_RETURN,        {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_SUPER,         {super_,   NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_THIS,          {this_,    NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_TRUE,          {literal,  NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_VAR,           {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_WHILE,         {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_ERROR,         {NULL,     NULL,   PREC_NONE}},
  {lox::TokenType::TOKEN_EOF,           {NULL,     NULL,   PREC_NONE}},
};


static void parsePrecedence(Precedence precedence) {
  parser->advance();
  ParseFn prefixRule = getRule(parser->getPreviousToken().getType())->prefix;
  if (prefixRule == NULL) {
    parser->parseError("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser->getCurrentToken().getType())->precedence) {
    parser->advance();
    ParseFn infixRule = getRule(parser->getPreviousToken().getType())->infix;
    infixRule(canAssign);
  }

  if (canAssign && parser->parseOptional(lox::TokenType::TOKEN_EQUAL)) {
    parser->parseError("Invalid assignment target.");
  }
}

static uint64_t identifierConstant(lox::Token& name) {
  std::string_view str = name.getTokenString();
  return makeConstant(OBJ_VAL(copyString(str.data(), str.length())));
}

static int resolveLocal(Compiler* compiler, lox::Token& name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, local->name)) {
      if (local->depth == -1) {
        parser->parseError("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index,
    bool isLocal) {
  int upvalueCount = compiler->function->upvalueCount;
  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    parser->parseError("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, lox::Token& name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
  }

  return -1;
}

static bool identifiersEqual(lox::Token& a, lox::Token& b) {
  std::string_view strA = a.getTokenString();
  std::string_view strB = b.getTokenString();
  if (strA.length() != strB.length()) return false;
  return strA.compare(strB) == 0;
}

static void addLocal(lox::Token name) {
  if (current->localCount == UINT8_COUNT) {
    parser->parseError("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;
}

static void declareVariable() {
  if (current->scopeDepth == 0) return;

  lox::Token& name = parser->getPreviousToken();
  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break;
    }

    if (identifiersEqual(name, local->name)) {
      parser->parseError("Already a variable with this name in this scope.");
    }
  }

  addLocal(name);
}

static uint64_t parseVariable() {
  parser->parse(lox::TokenType::TOKEN_IDENTIFIER);
  declareVariable();
  if (current->scopeDepth > 0) return 0;
  return identifierConstant(parser->getPreviousToken());
}

static void markInitialized() {
  if (current->scopeDepth == 0) return;
  current->locals[current->localCount - 1].depth =
      current->scopeDepth;
}

static void defineVariable(uint64_t global) {
  if (current->scopeDepth > 0) {
    markInitialized();
    return;
  }

  if (global > UINT8_MAX) {
    emitBytes(OP_DEFINE_GLOBAL_LONG, global >> 16);
    emitBytes(global >> 8, global);
  }
  else {
    emitBytes(OP_DEFINE_GLOBAL, global);
  }
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
  if (!parser->match(lox::TokenType::TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        parser->parseError("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (parser->parseOptional(lox::TokenType::TOKEN_COMMA));
  }
  parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
  return argCount;
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
  while (!parser->match(lox::TokenType::TOKEN_RIGHT_BRACE) && !parser->match(lox::TokenType::TOKEN_EOF)) {
    declaration();
  }

  parser->parse(lox::TokenType::TOKEN_RIGHT_BRACE);
}

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope();

  parser->parse(lox::TokenType::TOKEN_LEFT_PAREN);
  if (!parser->match(lox::TokenType::TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        parser->parseError("Can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable();
      defineVariable(constant);
    } while (parser->parseOptional(lox::TokenType::TOKEN_COMMA));
  }
  parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
  parser->parse(lox::TokenType::TOKEN_LEFT_BRACE);
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method() {
  parser->parse(lox::TokenType::TOKEN_IDENTIFIER);
  lox::Token &name = parser->getPreviousToken();
  uint8_t constant = identifierConstant(name);
  FunctionType type = TYPE_METHOD;
  if (name == "init") {
    type = TYPE_INITIALIZER;
  }
  function(type);
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  parser->parse(lox::TokenType::TOKEN_IDENTIFIER);
  lox::Token className = parser->getPreviousToken();
  uint8_t nameConstant = identifierConstant(parser->getPreviousToken());
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  ClassCompiler classCompiler;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;
  classCompiler.hasSuperclass = false;

  if (parser->parseOptional(lox::TokenType::TOKEN_LESS)) {
    parser->parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);

    if (identifiersEqual(className, parser->getPreviousToken())) {
      parser->parseError("A class can't inherit from itself.", false);
    }

    beginScope();
    addLocal(lox::Token("super"));
    defineVariable(0);

    namedVariable(className, false);
    emitByte(OP_INHERIT);
    classCompiler.hasSuperclass = true;
  }

  namedVariable(className, false);
  parser->parse(lox::TokenType::TOKEN_LEFT_BRACE);
  while (!parser->match(lox::TokenType::TOKEN_RIGHT_BRACE) && !parser->match(lox::TokenType::TOKEN_EOF)) {
    method();
  }
  parser->parse(lox::TokenType::TOKEN_RIGHT_BRACE);
  emitByte(OP_POP);

  if (classCompiler.hasSuperclass) {
    endScope();
  }
  currentClass = currentClass->enclosing;
}

static void funDeclaration() {
  uint8_t global = parseVariable();
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

static void varDeclaration() {
  uint64_t global = parseVariable();

  if (parser->parseOptional(lox::TokenType::TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  parser->parse(lox::TokenType::TOKEN_SEMICOLON);

  defineVariable(global);
}

static void expressionStatement() {
  expression();
  parser->parse(lox::TokenType::TOKEN_SEMICOLON);
  emitByte(OP_POP);
}

static void forStatement() {
  beginScope();
  parser->parse(lox::TokenType::TOKEN_LEFT_PAREN);
  if (parser->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (parser->parseOptional(lox::TokenType::TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  if (parser->getPreviousToken().getType() != lox::TokenType::TOKEN_SEMICOLON) {
    parser->synchronize();
  }

  int loopStart = currentChunk()->count;
  int exitJump = -1;
  if (!parser->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
    expression();
    parser->parse(lox::TokenType::TOKEN_SEMICOLON);

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!parser->parseOptional(lox::TokenType::TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);
    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endScope();
}

static void ifStatement() {
  parser->parse(lox::TokenType::TOKEN_LEFT_PAREN);
  expression();
  parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (parser->parseOptional(lox::TokenType::TOKEN_ELSE)) {
    statement();
  }
  patchJump(elseJump);
}

static void printStatement() {
  expression();
  parser->parse(lox::TokenType::TOKEN_SEMICOLON);
  emitByte(OP_PRINT);
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    parser->parseError("Can't return from top-level code.");
  }

  if (parser->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    if (current->type == TYPE_INITIALIZER) {
      parser->parseError("Can't return a value from an initializer.");
    }

    expression();
    parser->parse(lox::TokenType::TOKEN_SEMICOLON);
    emitByte(OP_RETURN);
  }
}

static void whileStatement() {
  int loopStart = currentChunk()->count;
  parser->parse(lox::TokenType::TOKEN_LEFT_PAREN);
  expression();
  parser->parse(lox::TokenType::TOKEN_RIGHT_PAREN);

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
}

static void declaration() {
  if (parser->parseOptional(lox::TokenType::TOKEN_CLASS)) {
    classDeclaration();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_FUN)) {
    funDeclaration();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  parser->synchronize();
}

static void statement() {
  if (parser->parseOptional(lox::TokenType::TOKEN_PRINT)) {
    printStatement();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_IF)) {
    ifStatement();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_RETURN)) {
    returnStatement();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_WHILE)) {
    whileStatement();
  } else if (parser->parseOptional(lox::TokenType::TOKEN_FOR)) {
    forStatement();
  } else {
    expressionStatement();
  }
}

ObjFunction* compile(const char* source) {
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT);

  parser = lox::Parser(source);

  parser->advance();

  while (parser->hasNext()) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  return parser->hasError() ? NULL : function;
}

void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
