#ifndef LEXER_H
#define LEXER_H

#include "../utils/arena.h"
#include "config.h"

typedef struct {
    Arena* arena;
    CatalyzeConfig* config;
    const char* buffer;
    size_t len;
    size_t current;
    char c;
} Lexer;

Result lexer_parse(Arena* arena, const char* buffer);

#endif // !LEXER_H
