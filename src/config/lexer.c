#include "lexer.h" 
#include "config.h"

#include <stdio.h>
#include <string.h>

#define IS_ALPHA(c) (char_map[(char)(c)] & 1)
#define IS_WHITESPACE(c) (char_map[(char)(c)] & 2)
#define IS_IDENTIFIER(c) (char_map[(char)(c)] & 4)
#define IS_DELIM(c) (char_map[(char)(c)] & 8)

static const uint8_t char_map[256] = {
    ['0' ... '9'] = 1,

    ['a' ... 'z'] = 1,
    ['A' ... 'Z'] = 1,
    ['_'] = 1,
    ['-'] = 1,

    [' '] = 2, 
    ['\t'] = 2, 
    ['\n'] = 2,

    [':'] = 4,

    ['{'] = 8,
    ['}'] = 8,
};

static void lexer_err(Lexer* lexer, const char* msg) {
    int line = 1;
    int col = 1;

    for (int i = 0; i < lexer -> current; i++) {
        if (lexer -> buffer[i] == '\n') {
            col = 1;
            line++;
        } else {
            col++;
        }
    }

    printf("\e[1mError in config.cat at line %d, column %d:\e[0m %s\n", line, col, msg);
}

static Lexer* create_lexer(Arena* arena, CatalyzeConfig* config, const char* buffer) {
    Lexer* lexer = arena_alloc(arena, sizeof(*lexer));

    lexer -> arena = arena;
    lexer -> config = config;
    lexer -> buffer = buffer;
    lexer -> len = strlen(lexer -> buffer);
    lexer -> current = 0;
    lexer -> c = lexer -> buffer[0];

    return lexer;
}

static void advance(Lexer* lexer) {
    if (lexer -> current >= lexer -> len) {
        lexer -> c = '\0';
        return;
    }

    lexer -> current++;
    if (lexer -> current >= lexer -> len) {
        lexer -> c = '\0';
    } else {
        lexer -> c = lexer -> buffer[lexer -> current];
    }
}

static void skip_whitespace(Lexer* lexer) {
    while (IS_WHITESPACE(lexer -> c)) {
        if (lexer -> current >= lexer -> len) return;
        advance(lexer);
    } 
}

Result lexer_parse(Arena* arena, const char* buffer) {
    CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
    Lexer* lexer = create_lexer(arena, config, buffer);

    skip_whitespace(lexer);
    if (lexer -> c != 'c') {
        lexer_err(lexer, "Expected 'config'");
        return err("Parsing failed, error in config.cat");
    }

    return ok("placeholder");
}
