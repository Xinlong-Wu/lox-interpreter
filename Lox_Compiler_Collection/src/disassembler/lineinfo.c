#include "chunk.h"
#include "common.h"
#include "disassembler/lineinfo.h"
#include "memory.h"

LineInfo createLineInfo(int line, int column)
{
    return (LineInfo){line, column, 0};
}

LineInfo getLineInfo(Chunk *chunk, int offset)
{
    LineInfoArray lineinfos = chunk->lineinfos;
    int arraySize = lineinfos.count;
    int mid = arraySize / 2;
    int low = 0;
    int high = arraySize - 1;

    while (low <= high)
    {
        mid = low + (high - low) / 2;
        if (offset < lineinfos.lineinfos[mid].offset)
        {
            high = mid - 1;
        }
        else if (offset > lineinfos.lineinfos[mid].offset)
        {
            low = mid + 1;
        }
        else
        {
            break;
        }
    }

    return lineinfos.lineinfos[mid];
}

bool isSameLineInfo(LineInfo a, LineInfo b){
    return a.line == b.line && a.column == b.column;
}

void initLineInfoArray(LineInfoArray *array)
{
    array->capacity = 0;
    array->count = 0;
    array->lineinfos = NULL;
}

void writeLineInfoArray(LineInfoArray *array, LineInfo lineinfo)
{
    if (array->capacity < array->count + 1)
    {
        int oldCapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldCapacity);
        array->lineinfos = GROW_ARRAY(LineInfo, array->lineinfos, oldCapacity, array->capacity);
    }

    array->lineinfos[array->count] = lineinfo;
    array->count++;
}

void tryAppendUniqueLineInfo(LineInfoArray *array, LineInfo lineinfo)
{
    int prevLineInfoIdx = array->count - 1;
    if (array->count == 0 || !isSameLineInfo(array->lineinfos[prevLineInfoIdx], lineinfo))
    {
        writeLineInfoArray(array, lineinfo);
    }
}

void freeLineInfoArray(LineInfoArray *array){
    FREE_ARRAY(LineInfo, array->lineinfos, array->capacity);
    initLineInfoArray(array);
}
