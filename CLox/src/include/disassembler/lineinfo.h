#ifndef clox_lineinfo_h
#define clox_lineinfo_h

#include "common.h"

typedef struct _LineInfo
{
    int line;
    int column;
    int offset;
} LineInfo;

typedef struct _LineInfoArray
{
    int capacity;
    int count;
    LineInfo *lineinfos;
} LineInfoArray;

LineInfo createLineInfo(int line, int column);
LineInfo getLineInfo(struct _Chunk *chunk, int offset);
bool isSameLineInfo(LineInfo a, LineInfo b);

void initLineInfoArray(LineInfoArray *array);
void tryAppendUniqueLineInfo(LineInfoArray *array, LineInfo lineinfo);
void freeLineInfoArray(LineInfoArray *array);

#endif
