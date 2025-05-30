#ifndef clox_vm_h
#define clox_vm_h

#include "value.h"
#include "chunk.h"
#include "table.h"
#include "vm/object.h"

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
} CallFrame;

typedef struct VM{
  CallFrame frames[FRAMES_MAX];
  int frameCount;

  Value stack[STACK_MAX];
  Value* stackTop;
  Table globals;
  Table strings;
  ObjString* initString;
  ObjUpvalue* openUpvalues;
  size_t bytesAllocated;
  size_t nextGC;
  Obj* objects;
  int grayCount;
  int grayCapacity;
  Obj** grayStack;
} VM;

typedef enum _InterpretResult {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

extern VM vm;

void initVM();
void freeVM();
InterpretResult interpret(const char* source);
void push(Value value);
Value pop();

#ifdef __cplusplus
}
#endif
#endif
