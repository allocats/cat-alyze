#include "lexer.h" 
#include "config.h"

#include <stddef.h>
#include <stdint.h>
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
    ['/'] = 1,
    ['.'] = 1,

    [' '] = 2, 
    ['\t'] = 2, 
    ['\n'] = 2,

    [':'] = 4,

    ['{'] = 8,
    ['}'] = 8,
};

static void lexer_err(Lexer* lexer, const char* msg) {
    int32_t line = 1;
    int32_t col = 1;

    for (int32_t i = 0; i < lexer -> current; i++) {
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

    lexer -> config -> flag_count = 0;
    lexer -> config -> source_count = 0;

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

static Result parse_single(Lexer* lexer, char* dest, size_t max_len) {
    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    size_t len = &lexer -> buffer[lexer -> current] - start;

    if (len >= max_len) {
        lexer_err(lexer, "Input too long");
        return err("Parsing failed");
    }

    strncpy(dest, start, len);
    dest[len] = '\0';
    return ok(NULL);
}

static Result parse_flags(Lexer* lexer) {
    uint8_t i = lexer -> config -> flag_count;

    while (lexer -> c != '\n' && i < MAX_FLAGS) {
        skip_whitespace(lexer);
        if (lexer -> c == '\n') break;

        const char* flag_start = &lexer -> buffer[lexer -> current];
        while (IS_ALPHA(lexer -> c)) {
            advance(lexer);
        }

        size_t flag_len = &lexer -> buffer[lexer -> current] - flag_start;
        if (flag_len > MAX_FLAG_LEN || flag_start[0] != '-') {
            lexer_err(lexer, "Invalid flag");
            return err("Invalid flag");
        }

        strncpy(lexer -> config -> flags[i], flag_start, flag_len);
        lexer -> config -> flags[i][flag_len] = '\0';
        i++;
    }

    lexer -> config -> flag_count = i;
    return ok(NULL);
}

static Result match_options(Lexer* lexer, const char* start, size_t len) {
    advance(lexer);
    skip_whitespace(lexer);

    switch (start[0]) {
        case 'b':
            if (strncmp(start, "build_dir", len) == 0) return parse_single(lexer, lexer -> config -> build_dir, MAX_BUILD_DIR_LEN);
            break;

        case 'c':
            if (strncmp(start, "compiler", len) == 0) return parse_single(lexer, lexer -> config -> compiler, MAX_COMPILER_LEN); 
            break;

        case 'f':
            if (strncmp(start, "flags", len) == 0) return parse_flags(lexer);
            break;
    }

    lexer_err(lexer, "Unknown option");
    return err("Unknown option");
}

static Result parse_key(Lexer* lexer) {
    skip_whitespace(lexer);

    if (lexer -> c == '}') {
        return ok(NULL);
    }

    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    size_t len = &lexer -> buffer[lexer -> current] - start;
    return match_options(lexer, start, len);
}

Result lexer_parse(Arena* arena, const char* buffer) {
    CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
    Lexer* lexer = create_lexer(arena, config, buffer);

    skip_whitespace(lexer);
    if (lexer -> c != 'c') {
        lexer_err(lexer, "Expected 'config'");
        return err("Parsing failed, error in config.cat");
    }

    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    size_t len = &lexer -> buffer[lexer -> current] - start;
    if (strncmp(start, "config", len) != 0) {
        lexer_err(lexer, "Expected 'config'");
        return err("Parsing failed, error in config.cat");
    }
    skip_whitespace(lexer);

    if (lexer -> c != '{') {
        lexer_err(lexer, "Expected '{'");
        return err("Parsing failed, error in config.cat");
    }
    
    advance(lexer);
    skip_whitespace(lexer);

    while (lexer -> c != '}') {
        Result result = parse_key(lexer);
        if (IS_ERR(result)) {
            printf("%s\n", ERR_MSG(result));
            return err("Parsing failed, error in config.cat");
        }
    } 

    advance(lexer);
    skip_whitespace(lexer);

    if (lexer -> c != 't') {
        lexer_err(lexer, "Expected 'target'");
        return err("Parsing failed, error in config.cat");
    }

    start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    len = &lexer -> buffer[lexer -> current] - start;
    if (strncmp(start, "target", len) != 0) {
        lexer_err(lexer, "Expected 'target'");
        return err("Parsing failed, error in config.cat");
    }
    skip_whitespace(lexer);

    start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    len = &lexer -> buffer[lexer -> current] - start;
    char* target;
    strncpy(target, start, len);
    target[len] = '\0';

    skip_whitespace(lexer);
    if (lexer -> c != '{') {
        lexer_err(lexer, "Expected '{'");
        return err("Parsing failed, error in config.cat");
    }
    
    advance(lexer);
    skip_whitespace(lexer);

    while (lexer -> c != '}') {
        Result result = parse_key(lexer);
        if (IS_ERR(result)) {
            printf("%s\n", ERR_MSG(result));
            return err("Parsing failed, error in config.cat");
        }
    } 


    return ok(lexer -> config);
}
