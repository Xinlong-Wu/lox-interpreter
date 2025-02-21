#include <stdio.h>

#include "disassembler/debug.h"
#include "value.h"

void disassembleChunk(Chunk *chunk, const char *name)
{
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->count;)
  {
    offset = disassembleInstruction(chunk, offset);
  }
}

int constantInstruction(const char *name, Chunk *chunk,
                        int offset)
{
  uint8_t instruction = chunk->code[offset];
  uint8_t len = 1;
  uint64_t constant = 0;

  if (instruction == OP_CONSTANT_LONG)
    len = 3;

  for (size_t i = 1; i <= len; i++)
  {
    constant = (constant << 8) + chunk->code[offset + i];
  }
  printf("%-16s\t%4lu\t'", name, constant);
  printValue(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 1 + len;
}

int simpleInstruction(const char *name, int offset)
{
  printf("%s\n", name);
  return offset + 1;
}

int disassembleInstruction(Chunk *chunk, int offset)
{
  printf("%04d ", offset);

  LineInfo line = getLineInfo(chunk, offset);
  printf("[%3d:%3d]\t", line.line, line.column);

  uint8_t instruction = chunk->code[offset];
  switch (instruction)
  {
  case OP_CONSTANT:
    return constantInstruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return constantInstruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_ADD:
    return simpleInstruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simpleInstruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simpleInstruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simpleInstruction("OP_DIVIDE", offset);
  case OP_NEGATE:
    return simpleInstruction("OP_NEGATE", offset);
  case OP_RETURN:
    return simpleInstruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}
