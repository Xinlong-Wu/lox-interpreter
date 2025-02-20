#include "common.h"
#include "chunk.h"
#include "disassembler/debug.h"
#include "disassembler/lineinfo.h"

int main(int argc, const char *argv[])
{
    Chunk chunk;
    initChunk(&chunk);
    for (size_t i = 0; i < 260; i++)
    {
        writeConstant(&chunk, 1.2, createLineInfo(i,1));
    }
    writeChunk(&chunk, OP_RETURN, createLineInfo(1,2));
    disassembleChunk(&chunk, "test chunk");
    freeChunk(&chunk);
    return 0;
}