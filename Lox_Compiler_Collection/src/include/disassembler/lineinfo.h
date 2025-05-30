#ifndef clox_lineinfo_h
#define clox_lineinfo_h

#include "_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LineInfo
{
    int line;
    int column;
    int offset;
} LineInfo;

typedef struct LineInfoArray
{
    int capacity;
    int count;
    LineInfo *lineinfos;
} LineInfoArray;

LineInfo createLineInfo(int line, int column);
LineInfo getLineInfo(struct Chunk *chunk, int offset);
bool isSameLineInfo(LineInfo a, LineInfo b);

void initLineInfoArray(LineInfoArray *array);
void tryAppendUniqueLineInfo(LineInfoArray *array, LineInfo lineinfo);
void freeLineInfoArray(LineInfoArray *array);

#ifdef __cplusplus
}
#endif
#endif
