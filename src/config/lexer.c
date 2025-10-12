#include "lexer.h" 

#include "config.h"
#include "char_map.h"
#include "../utils/macros.h"
#include "config_hashes.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parse_config_section(Lexer* lexer);
static void parse_compiler(Lexer* lexer);
static void parse_build_dir(Lexer* lexer);
static void parse_default_flags(Lexer* lexer);

static void parse_target_type(Lexer* lexer);
static void parse_target_name(Lexer* lexer);
static void parse_sources(Lexer* lexer);
static void parse_flags(Lexer* lexer);
static void parse_output(Lexer* lexer);

static const FieldHandler fields[] = {
    { CONFIG_HASH, parse_config_section }, 
    { COMPILER_HASH, parse_compiler },
    { BUILD_DIR_HASH, parse_build_dir },
    { DEFAULT_FLAGS_HASH, parse_default_flags },
    { SOURCES_HASH, parse_sources },
    { FLAGS_HASH, parse_flags },
    { OUTPUT_HASH, parse_output },
    { 0, NULL }
};

void lexer_err(Lexer* lexer, const char* msg) {
    int32_t line = 1;
    int32_t col = 1;

    const char* cursor = lexer -> cursor;
    const char* buffer = lexer -> buffer;

    size_t len = cursor - buffer;

    for (size_t i = 0; i < len; i++) {
        if (buffer[i] == '\n') {
            col = 1;
            line++;
        } else {
            col++;
        }
    }

    printf("\033[1mError in config.cat at line %d, column %d:\033[0m %s\n", line, col, msg);
    exit(1);
}

static inline uint32_t djb2_hash(const char* s) {
    uint32_t hash = 5381;
    while (*s) {
        hash = ((hash << 5) + hash) + *s++;
    }
    return hash;
}
 
static inline Lexer* create_lexer(ArenaAllocator* arena, CatalyzeConfig* config, char* buffer, const size_t size) {
    Lexer* lexer = arena_alloc(arena, sizeof(*lexer));

    lexer -> arena = arena;
    lexer -> config = config;
    lexer -> buffer = buffer;
    lexer -> cursor = lexer -> buffer;
    lexer -> end = lexer -> buffer + size;

    return lexer;
}

static inline void advance(Lexer* lexer) {
    lexer -> cursor = ++lexer -> cursor >= lexer -> end ? lexer -> end : lexer -> cursor;
}

#define ADVANCE_CURSOR(cursor, end) cursor = ++cursor >= end ? end : cursor; \
    if (cursor == end) lexer_err(lexer, "Sudden eof!")

static inline void skip_whitespace(Lexer* lexer) {
    while (IS_WHITESPACE(*(lexer -> cursor))) {
        advance(lexer);
    } 
}

static void parse_config_section(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* end = lexer -> end;
    
    if (*cursor != '{') {
        lexer_err(lexer, "Expected '{'!");
    }

    ADVANCE_CURSOR(cursor, end);
    lexer -> cursor = cursor;
    skip_whitespace(lexer);
    cursor = lexer -> cursor;

    while (*cursor != '}') {
        skip_whitespace(lexer);
        cursor = lexer -> cursor;
        if (*cursor == '}') {
            *cursor = 0;
            cursor++;
            lexer -> cursor = cursor;
            return;
        }

        char* start = cursor;
        while (IS_ALPHA(*cursor)) {
            ADVANCE_CURSOR(cursor, end);
        }

        *cursor = 0;
        cursor++;

        lexer -> cursor = cursor;

        uint32_t hash = djb2_hash(start);
        bool found = false;
        for (const FieldHandler* field = fields; field -> hash != 0; field++) {
            if (field -> hash == hash) {
                field -> fn(lexer);
                cursor = lexer -> cursor;
                found = true;
                break;
            }
        }

        if (!found) {
            printf("Error: %s\n", start);
            lexer_err(lexer, "Unknown field in config section");
        }
    }

    cursor++;
    lexer->cursor = cursor;
}

static void parse_compiler(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

    lexer -> config -> compiler = start;
    lexer -> cursor = cursor;
}

static void parse_build_dir(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

    lexer -> config -> build_dir = start;
    lexer -> cursor = cursor;
}

static void parse_default_flags(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* end = lexer -> end;

    if (*cursor++ != '[') {
        lexer_err(lexer, "Expected '['!");
    }

    char* start = cursor;
    while (*cursor != ']') {
        if (*cursor == 0) lexer_err(lexer, "Expected ']'!");
        if (*cursor == '\t' || *cursor == '\n') *cursor = ' ';
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;
    lexer -> cursor = cursor;

    while (*start != 0) {
        while (IS_WHITESPACE(*start)) {
            start++;
        }

        if (*start == 0) return;

        char* flag_start = start;

        while (IS_ALPHA(*start)) {
            start++;
        }

        if (*start == 0) {
            lexer -> config -> default_flags[lexer -> config -> default_flag_count++] = flag_start;
            return;
        }

        *start = 0;
        start++;
        lexer -> config -> default_flags[lexer -> config -> default_flag_count++] = flag_start;
    }
}

static void parse_target(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    if (cursor >= end) return;
    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

    if (cursor >= end) return;

    uint32_t hash = djb2_hash(start);
    if (hash != TARGET_HASH) {
        lexer_err(lexer, "Expected 'target'!");
    }

    lexer -> cursor = cursor;

    parse_target_type(lexer);
    parse_target_name(lexer);
    skip_whitespace(lexer);

    cursor = lexer -> cursor;
    if (*cursor != '{') {
        lexer_err(lexer, "Expected '{'!");
    }

    ADVANCE_CURSOR(cursor, end);
    lexer -> cursor = cursor;
    skip_whitespace(lexer);
    cursor = lexer -> cursor;

    while (*cursor != '}') {
        skip_whitespace(lexer);
        cursor = lexer -> cursor;
        if (*cursor == '}') {
            *cursor = 0;
            cursor++;
            lexer -> cursor = cursor;
            lexer -> config -> target_count++;
            return;
        }

        char* start = cursor;
        while (IS_ALPHA(*cursor)) {
            ADVANCE_CURSOR(cursor, end);
        }

        *cursor = 0;
        cursor++;

        lexer -> cursor = cursor;

        uint32_t hash = djb2_hash(start);
        bool found = false;
        for (const FieldHandler* field = fields; field -> hash != 0; field++) {
            if (field -> hash == hash) {
                field -> fn(lexer);
                cursor = lexer -> cursor;
                found = true;
                break;
            }
        }

        if (!found) {
            printf("Error: %s\n", start);
            lexer_err(lexer, "Unknown field in target");
        }
    }

    cursor++;
    lexer -> cursor = cursor;
    lexer -> config -> target_count++;
}

static void parse_target_type(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

    uint32_t hash = djb2_hash(start);
    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];

    switch (hash) {
        case EXECUTABLE_HASH: {
            target -> type = Executable;
            break;
        }

        case DEBUG_HASH: {
            target -> type = Debug;
            break;
        }

        case TEST_HASH: {
            target -> type = Test;
            break;
        }

        default: {
            lexer_err(lexer, "Unknown target type");
        }
    }

    lexer -> cursor = cursor;
}

static void parse_target_name(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    target -> name = start;
    lexer -> cursor = cursor;
}

static void parse_sources(Lexer* lexer) {
    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    target -> source_count = 0;

    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* end = lexer -> end;

    if (*cursor++ != '[') {
        lexer_err(lexer, "Expected '['!");
    }

    char* start = cursor;
    while (*cursor != ']') {
        if (*cursor == 0) lexer_err(lexer, "Expected ']'!");
        if (*cursor == '\t' || *cursor == '\n') *cursor = ' ';
        ADVANCE_CURSOR(cursor, end);
    }


    *cursor = 0;
    cursor++;
    lexer -> cursor = cursor;

    while (*start != 0) {
        while (IS_WHITESPACE(*start)) {
            start++;
        }

        if (*start == 0) return;

        char* source_start = start;

        while (IS_ALPHA(*start)) {
            start++;
        }

        if (*start == 0) {
            target -> sources[target -> source_count++] = source_start;
            return;
        }

        *start = 0;
        start++;
        target -> sources[target -> source_count++] = source_start;
    }
}

static void parse_flags(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* end = lexer -> end;

    if (*cursor++ != '[') {
        lexer_err(lexer, "Expected '['!");
    }

    char* start = cursor;
    while (*cursor != ']') {
        if (*cursor == 0) lexer_err(lexer, "Expected ']'!");
        if (*cursor == '\t' || *cursor == '\n') *cursor = ' ';
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;
    lexer -> cursor = cursor;

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];

    while (*start != 0) {
        while (IS_WHITESPACE(*start)) {
            start++;
        }

        if (*start == 0) return;

        char* flag_start = start;

        while (IS_ALPHA(*start)) {
            start++;
        }

        if (*start == 0) {
            target -> flags[target -> flag_count++] = flag_start;
            return;
        }

        *start = 0;
        start++;
        target -> flags[target -> flag_count++] = flag_start;
    }
}

static void parse_output(Lexer* lexer) {
    skip_whitespace(lexer);
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* buffer_end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, buffer_end);
    }

    char* end = cursor;
    char* current = end;

    if (UNLIKELY(start == end)) {
        lexer_err(lexer, "Invalid target output path");
    }

    while (*(current - 1) != '/') {
        if (current == start) {
            lexer_err(lexer, "Invalid output");
        }
        current--;
    }

    cursor = current;
    cursor--;
    *cursor = 0;

    cursor = end;
    *cursor = 0;
    cursor++;

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    target -> output_dir = start;
    target -> output_name = current; 

    lexer -> cursor = cursor;
}

CatalyzeConfig* lexer_parse(ArenaAllocator* arena, char* buffer, const size_t size, const uint8_t nest_count) {
    CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
    arena_memset(config, 0, sizeof(*config));

    Lexer* lexer = create_lexer(arena, config, buffer, size);
    lexer -> config -> target_count = 0;
    lexer -> config -> nest_count = nest_count;

    skip_whitespace(lexer);

    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;
    lexer -> cursor = cursor;

    uint32_t hash = djb2_hash(start);
    if (hash != CONFIG_HASH) lexer_err(lexer, "Expected config section!"); 

    parse_config_section(lexer);
    skip_whitespace(lexer);

    while (lexer -> cursor < lexer -> end) {
        skip_whitespace(lexer);
        if (lexer -> cursor >= lexer -> end) break;

        parse_target(lexer);
    }

    return config;
}
