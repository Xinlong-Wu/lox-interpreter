#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm/object.h"
#include "vm/vm.h"

ObjFunction* compile(const char* source);
void markCompilerRoots();

#endif
