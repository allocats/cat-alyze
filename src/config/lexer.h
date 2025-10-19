#ifndef LEXER_H
#define LEXER_H

#include "../utils/arena.h"
#include "config.h"
#include "config_hashes.h"

#include <stdint.h>

typedef struct {
    char* buffer;
    char* cursor;
    char* end;
    ArenaAllocator* arena;
    CatalyzeConfig* config;
} Lexer;

typedef void (*HandlerFunc)(Lexer*);
typedef struct {
    uint32_t hash;
    HandlerFunc fn;
} FieldHandler;

CatalyzeConfig* lexer_parse(ArenaAllocator* arena, char* buffer, const size_t size, const char* prefix, size_t path_len);

#endif // !LEXER_H
