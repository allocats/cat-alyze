#include "lexer.h" 

#include "config.h"
#include "../utils/macros.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define IS_ALPHA(c) (char_map[(unsigned char)(c)] & 1)
#define IS_WHITESPACE(c) (char_map[(unsigned char)(c)] & 2)
#define IS_IDENTIFIER(c) (char_map[(unsigned char)(c)] & 4)
#define IS_DELIM(c) (char_map[(unsigned char)(c)] & 8)

static const uint8_t char_map[256] = {
    ['0'] = 1,
    ['1'] = 1,
    ['2'] = 1,
    ['3'] = 1,
    ['4'] = 1,
    ['5'] = 1,
    ['6'] = 1,
    ['7'] = 1,
    ['8'] = 1,
    ['9'] = 1,

    ['a'] = 1,
    ['b'] = 1,
    ['c'] = 1,
    ['d'] = 1,
    ['e'] = 1,
    ['f'] = 1,
    ['g'] = 1,
    ['h'] = 1,
    ['i'] = 1,
    ['j'] = 1,
    ['k'] = 1,
    ['l'] = 1,
    ['m'] = 1,
    ['n'] = 1,
    ['o'] = 1,
    ['p'] = 1,
    ['q'] = 1,
    ['r'] = 1,
    ['s'] = 1,
    ['t'] = 1,
    ['u'] = 1,
    ['v'] = 1,
    ['w'] = 1,
    ['x'] = 1,
    ['y'] = 1,
    ['z'] = 1,

    ['A'] = 1,
    ['B'] = 1,
    ['C'] = 1,
    ['D'] = 1,
    ['E'] = 1,
    ['F'] = 1,
    ['G'] = 1,
    ['H'] = 1,
    ['I'] = 1,
    ['J'] = 1,
    ['K'] = 1,
    ['L'] = 1,
    ['M'] = 1,
    ['N'] = 1,
    ['O'] = 1,
    ['P'] = 1,
    ['Q'] = 1,
    ['R'] = 1,
    ['S'] = 1,
    ['T'] = 1,
    ['U'] = 1,
    ['V'] = 1,
    ['W'] = 1,
    ['X'] = 1,
    ['Y'] = 1,
    ['Z'] = 1,

    ['_'] = 1,
    ['-'] = 1,
    ['/'] = 1,
    ['.'] = 1,
    ['*'] = 1,
    ['='] = 1,

    [' '] = 2, 
    ['\t'] = 2, 
    ['\n'] = 2,

    [':'] = 4,

    ['{'] = 8,
    ['}'] = 8,
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

static inline Lexer* create_lexer(ArenaAllocator* arena, CatalyzeConfig* config, char* buffer) {
    Lexer* lexer = arena_alloc(arena, sizeof(*lexer));

    lexer -> arena = arena;
    lexer -> config = config;
    lexer -> len = strlen(buffer);
    lexer -> buffer = buffer;
    lexer -> cursor = lexer -> buffer;
    lexer -> end = lexer -> buffer + lexer -> len;

    return lexer;
}

static inline void advance(Lexer* lexer) {
    lexer -> cursor = ++lexer -> cursor >= lexer -> end ? lexer -> end : lexer -> cursor;
}

static inline void skip_whitespace(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    while (IS_WHITESPACE(**cursor)) {
        advance(lexer);
    } 
}

static inline void skip_spaces(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    while (IS_WHITESPACE(**cursor) && **cursor != '\n') {
        advance(lexer);
    } 
}

static inline void parse_single(Lexer* lexer, char** dest) {
    char** cursor = &lexer -> cursor;
    char* start = *cursor;
    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    (**cursor) = 0;
    (*cursor)++;

    set_single(lexer -> arena, dest, start);
}

static void parse_default_flags(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    uint8_t i = 0;

    while (**cursor != '\n' && i < MAX_FLAGS) {
        skip_spaces(lexer);
        if (**cursor == '\n') break;

        char* flag_start = *cursor;
        while (IS_ALPHA(**cursor)) {
            advance(lexer);
        }

        if (**cursor == '\n') {
            (**cursor) = 0;
            (*cursor)++;
            push_default_flag(lexer -> arena, lexer -> config, flag_start);
            break;
        }

        (**cursor) = 0;
        (*cursor)++;

        push_default_flag(lexer -> arena, lexer -> config, flag_start);
        i++;
    }
}

static void parse_target_flags(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    uint8_t count = 0;

    while (**cursor != '\n' && count < MAX_FLAGS) {
        skip_spaces(lexer);
        if (**cursor == '\n') break;

        char* flag_start = *cursor;
        while (IS_ALPHA(**cursor)) {
            advance(lexer);
        }

        if (**cursor == '\n') {
            (**cursor) = 0;
            (*cursor)++;
            push_flag(lexer -> arena, lexer -> config, flag_start);
            break;
        }

        (**cursor) = 0;
        (*cursor)++;

        push_flag(lexer -> arena, lexer -> config, flag_start);
        count++;
    }
}

static void parse_sources(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    uint8_t count = 0;

    while (**cursor != '\n' && count < MAX_SOURCES) {
        skip_spaces(lexer);
        if (**cursor == '\n') break;

        char* source_start = *cursor;
        while (IS_ALPHA(**cursor)) {
            advance(lexer);
        }

        if (**cursor == '\n') {
            (**cursor) = 0;
            (*cursor)++;
            push_source(lexer -> arena, lexer -> config, source_start);
            break;
        }

        (**cursor) = 0;
        (*cursor)++;

        push_source(lexer -> arena, lexer -> config, source_start);
        count++;
    }
}

static void parse_target_output(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    char* start = *cursor;
    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    char* end = *cursor;
    char* current = end;

    if (UNLIKELY(current == start)) {
        lexer_err(lexer, "Invalid output");
    }

    while (*(current - 1) != '/') {
        if (current == start) {
            lexer_err(lexer, "Invalid output");
        }
        current--;
    }

    *cursor = current;
    (*cursor)--;
    (**cursor) = 0;

    *cursor = end;
    (**cursor) = 0;
    (*cursor)++;

    set_output_name(lexer -> arena, lexer -> config, current);
    set_output_dir(lexer -> arena, lexer -> config, start);
}

static void match_options(Lexer* lexer, const char* start) {
    advance(lexer);
    skip_whitespace(lexer);

    switch (start[0]) {
        case 'b':
            if (strcmp(start, "build_dir") == 0) {
                parse_single(lexer, &lexer -> config -> build_dir);
                return;
            }
            break;

        case 'c':
            if (strcmp(start, "compiler") == 0) {
                parse_single(lexer, &lexer -> config -> compiler); 
                return;
            }
            break;

        case 'd':
            if (strcmp(start, "default_flags") == 0) {
                parse_default_flags(lexer);
                return;
            }
            break;

        case 'f':
            if (strcmp(start, "flags") == 0) {
                parse_target_flags(lexer);
                return;
            }
            break;

        case 'o':
            if (strcmp(start, "output") == 0) {
                parse_target_output(lexer);
                return;
            }
            break;

        case 's':
            if (strcmp(start, "sources") == 0) {
                parse_sources(lexer);
                return;
            }
            break;
    }

    lexer_err(lexer, "Unknown option");
}

static inline void parse_key(Lexer* lexer) {
    char** cursor = &lexer -> cursor;
    skip_whitespace(lexer);

    if (**cursor == '}') {
        return;
    }

    const char* start = *cursor;
    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    (**cursor) = 0;
    (*cursor)++;

    match_options(lexer, start);
}

static inline void expect_keyword(Lexer* lexer, const char* keyword) {
    char** cursor = &lexer -> cursor;
    const char* start = *cursor;
    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    size_t len = *cursor - start;
    if (len != strlen(keyword) || strncmp(start, keyword, len) != 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Expected: %s", keyword);
        lexer_err(lexer, msg);
    }
}

static inline void expect_char(Lexer* lexer, const char expected) {
    char** cursor = &lexer -> cursor;

    if (**cursor != expected) {
        char msg[16];
        snprintf(msg, sizeof(msg), "Expected: %c", expected);
        lexer_err(lexer, msg);
    }

    advance(lexer);
}

static inline void parse_identifier(Lexer* lexer, char** dest) {
    char** cursor = &lexer -> cursor;
    char* start = *cursor;

    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    (**cursor) = 0;
    (*cursor)++;

    set_single(lexer -> arena, dest, start);
}

static inline void parse_config_section(Lexer* lexer) {
    char** cursor = &lexer -> cursor;

    expect_keyword(lexer, "config");
    skip_whitespace(lexer);

    expect_char(lexer, '{');

    skip_whitespace(lexer);
    while (**cursor != '}') {
        parse_key(lexer);
        skip_whitespace(lexer);
    }

    advance(lexer);
}

static void parse_target_section(Lexer* lexer) {
    char** cursor = &lexer -> cursor;

    expect_keyword(lexer, "target");
    skip_whitespace(lexer);

    char* start = *cursor;

    while (IS_ALPHA(**cursor)) {
        advance(lexer);
    }

    (**cursor) = 0;
    (*cursor)++;

    switch (*start) {
        case 'e':
            if (strcmp(start, "executable") == 0) {
                set_type(lexer -> arena, lexer -> config, Executable);
                break;
            }
            lexer_err(lexer, "uknown target type");

        case 't':
            if (strcmp(start, "test") == 0) {
                set_type(lexer -> arena, lexer -> config, Test);
                break;
            }
            lexer_err(lexer, "uknown target type");

        case 'd':
            if (strcmp(start, "debug") == 0) {
                set_type(lexer -> arena, lexer -> config, Debug);
                break;
            }
            lexer_err(lexer, "uknown target type");

        case 's':
            if (strcmp(start, "static_lib") == 0) {
                set_type(lexer -> arena, lexer -> config, StaticLib);
                break;
            } else if (strcmp(start, "shared_lib") == 0) {
                set_type(lexer -> arena, lexer -> config, SharedLib);
                break;
            }
            lexer_err(lexer, "uknown target type");

        default:
            lexer_err(lexer, "Uknown target type");
    }

    skip_whitespace(lexer);

    Target* target = lexer -> config -> targets[lexer -> config -> target_count];
    parse_identifier(lexer, &target -> name);

    skip_whitespace(lexer);
    expect_char(lexer, '{');
    skip_whitespace(lexer);

    while (**cursor != '}') {
        parse_key(lexer);
        skip_whitespace(lexer);
    }

    advance(lexer);
    lexer -> config -> target_count++;
}

CatalyzeConfig* lexer_parse(ArenaAllocator* arena, char* buffer, uint8_t nest_count) {
    CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
    arena_memset(config, 0, sizeof(*config));

    Lexer* lexer = create_lexer(arena, config, buffer);

    config -> target_count = 0;
    config -> default_flag_count = 0;
    config -> nest_count = nest_count;

    skip_whitespace(lexer);
    parse_config_section(lexer);

    char** cursor = &lexer -> cursor;
    skip_whitespace(lexer);

    while (**cursor == 't' && lexer -> config -> target_count < MAX_TARGETS) {
        parse_target_section(lexer);
        skip_whitespace(lexer);
    }

    if (UNLIKELY(lexer -> config -> target_count == 0)) {
        lexer_err(lexer, "At least one target is required");
    }

    return lexer -> config;
}
