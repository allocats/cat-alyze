#ifndef LEXER_H
#define LEXER_H

#include "../utils/arena.h"
#include "config.h"

#include <stdint.h>

typedef struct {
    char* buffer;
    char* cursor;
    char* end;
    ArenaAllocator* arena;
    CatalyzeConfig* config;
} Lexer;

CatalyzeConfig* lexer_parse(ArenaAllocator* arena, char* buffer, const size_t size, const uint8_t nest_count);

#endif // !LEXER_H
