#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"
#include "disassembler/lineinfo.h"

typedef enum
{
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN,
} OpCode;

typedef struct _Chunk
{
    int count;
    int capacity;
    uint8_t *code;
    LineInfoArray lineinfos;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);
void writeChunk(Chunk *chunk, uint8_t byte, LineInfo line);
void writeConstant(Chunk *chunk, Value value, LineInfo line);
void freeChunk(Chunk *chunk);
int addConstant(Chunk *chunk, Value value);

#endif
