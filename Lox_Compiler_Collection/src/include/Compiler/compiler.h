#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm/object.h"
#include "vm/vm.h"

#ifdef __cplusplus
extern "C" {
#endif

ObjFunction* compile(const char* source);
void markCompilerRoots();

#ifdef __cplusplus
}
#endif
#endif
