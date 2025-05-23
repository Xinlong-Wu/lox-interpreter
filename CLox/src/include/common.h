#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NAN_BOXING

extern bool debug;
#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC
#define UINT8_COUNT (UINT8_MAX + 1)

struct Chunk;

#ifdef __cplusplus
}
#endif
#endif
