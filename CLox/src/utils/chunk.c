#include <stdlib.h>

#include "chunk.h"
#include "disassembler/lineinfo.h"
#include "memory.h"
#include "value.h"

void initChunk(Chunk *chunk)
{
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  initLineInfoArray(&chunk->lineinfos);
  initValueArray(&chunk->constants);
}

void writeChunk(Chunk *chunk, uint8_t byte, LineInfo line)
{
  if (chunk->capacity < chunk->count + 1)
  {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
                             oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  line.offset = chunk->count;
  tryAppendUniqueLineInfo(&chunk->lineinfos, line);
  chunk->count++;
}

int addConstant(Chunk *chunk, Value value)
{
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}

void writeConstant(Chunk* chunk, Value value, LineInfo line)
{
  int constant = addConstant(chunk, value);
  if (IsLongConstant(constant)) {
    writeChunk(chunk, OP_CONSTANT_LONG, line);
    writeChunk(chunk, constant >> 16 , line);
    writeChunk(chunk, constant >> 8, line);
  }
  else
    writeChunk(chunk, OP_CONSTANT, line);

  writeChunk(chunk, constant, line);
}

void freeChunk(Chunk *chunk)
{
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  freeLineInfoArray(&chunk->lineinfos);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}
