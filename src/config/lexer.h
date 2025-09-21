#ifndef LEXER_H
#define LEXER_H

#include "../utils/arena.h"
#include "config.h"

#include <stdint.h>

typedef struct {
    char c;
    const char* buffer;
    size_t current;
    size_t len;
    ArenaAllocator* arena;
    CatalyzeConfig* config;
} Lexer;

Result lexer_parse(ArenaAllocator* arena, const char* buffer, uint8_t nest_count);

#endif // !LEXER_H
