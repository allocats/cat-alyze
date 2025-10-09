#include "lexer.h" 

#include "config.h"
#include "char_map.h"
#include "../utils/macros.h"
#include "config_hashes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint8_t config_parsed = 0;

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
    CONFIG_HASH, parse_config_section,
    COMPILER_HASH, parse_compiler,
    BUILD_DIR_HASH, parse_build_dir,
    DEFAULT_FLAGS_HASH, parse_default_flags,
    SOURCES_HASH, parse_sources,
    FLAGS_HASH, parse_flags,
    OUTPUT_HASH, parse_output,
    0, NULL
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
    char* cursor = lexer -> cursor;
    while (IS_WHITESPACE(*cursor)) {
        advance(lexer);
    } 
    lexer -> cursor = cursor;
}

static inline void skip_spaces(Lexer* lexer) {
    char* cursor = lexer -> cursor;
    while (IS_WHITESPACE(*cursor) && *cursor != '\n') {
        advance(lexer);
    } 
    lexer -> cursor = cursor;
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

    while (*cursor != '}') {
        skip_whitespace(lexer);
        cursor = lexer -> cursor;
        char* start = cursor;

        while (IS_ALPHA(*cursor)) {
            ADVANCE_CURSOR(cursor, end);
        }

        *cursor = 0;
        cursor++;

        lexer -> cursor = cursor;

        uint32_t hash = djb2_hash(start);
        for (const FieldHandler* field = fields; field -> hash != 0; field++) {
            if (field -> hash == hash) {
                field -> fn(lexer);
                break;
            }
        }
    }
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

    lexer -> config -> default_flags = start;
    lexer -> cursor = cursor;
}

static void parse_target(Lexer* lexer) {
    char* cursor = lexer -> cursor;
    char* start = cursor;
    char* end = lexer -> end;

    while (IS_ALPHA(*cursor)) {
        ADVANCE_CURSOR(cursor, end);
    }

    *cursor = 0;
    cursor++;

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

    while (*cursor != '}') {
        skip_whitespace(lexer);
        cursor = lexer -> cursor;
        char* start = cursor;

        while (IS_ALPHA(*cursor)) {
            ADVANCE_CURSOR(cursor, end);
        }

        *cursor = 0;
        cursor++;

        lexer -> cursor = cursor;

        uint32_t hash = djb2_hash(start);
        for (const FieldHandler* field = fields; field -> hash != 0; field++) {
            if (field -> hash == hash) {
                field -> fn(lexer);
                break;
            }
        }
    }
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

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    target -> sources = start;
    lexer -> cursor = cursor;
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

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    target -> flags = start;
    lexer -> cursor = cursor;
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
    config -> target_count = 0;

    Lexer* lexer = create_lexer(arena, config, buffer, size);

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
    if (hash != CONFIG_HASH) lexer_err(lexer, "Expected config section!"); 

    parse_config_section(lexer);

    skip_whitespace(lexer);

    while (lexer -> cursor != lexer -> end) {
        parse_target(lexer);
        lexer -> config -> target_count++;
    }

    return config;
}

// static inline void parse_single(Lexer* lexer, char** dest) {
//     char** cursor = &lexer -> cursor;
//     char* start = *cursor;
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     (**cursor) = 0;
//     (*cursor)++;
//
//     set_single(lexer -> arena, dest, start);
// }
//
// static void parse_default_flags(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//     uint8_t i = 0;
//
//     while (**cursor != '\n' && i < MAX_FLAGS) {
//         skip_spaces(lexer);
//         if (**cursor == '\n') break;
//
//         char* flag_start = *cursor;
//         while (IS_ALPHA(**cursor)) {
//             advance(lexer);
//         }
//
//         if (**cursor == '\n') {
//             (**cursor) = 0;
//             (*cursor)++;
//             push_default_flag(lexer -> arena, lexer -> config, flag_start);
//             break;
//         }
//
//         (**cursor) = 0;
//         (*cursor)++;
//
//         push_default_flag(lexer -> arena, lexer -> config, flag_start);
//         i++;
//     }
// }
//
// static void parse_target_flags(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//     uint8_t count = 0;
//
//     while (**cursor != '\n' && count < MAX_FLAGS) {
//         skip_spaces(lexer);
//         if (**cursor == '\n') break;
//
//         char* flag_start = *cursor;
//         while (IS_ALPHA(**cursor)) {
//             advance(lexer);
//         }
//
//         if (**cursor == '\n') {
//             (**cursor) = 0;
//             (*cursor)++;
//             push_flag(lexer -> arena, lexer -> config, flag_start);
//             break;
//         }
//
//         (**cursor) = 0;
//         (*cursor)++;
//
//         push_flag(lexer -> arena, lexer -> config, flag_start);
//         count++;
//     }
// }
//
// static void parse_sources(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//     uint8_t count = 0;
//
//     while (**cursor != '\n' && count < MAX_SOURCES) {
//         skip_spaces(lexer);
//         if (**cursor == '\n') break;
//
//         char* source_start = *cursor;
//         while (IS_ALPHA(**cursor)) {
//             advance(lexer);
//         }
//
//         if (**cursor == '\n') {
//             (**cursor) = 0;
//             (*cursor)++;
//             push_source(lexer -> arena, lexer -> config, source_start);
//             break;
//         }
//
//         (**cursor) = 0;
//         (*cursor)++;
//
//         push_source(lexer -> arena, lexer -> config, source_start);
//         count++;
//     }
// }
//
// static void parse_target_output(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//     char* start = *cursor;
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     char* end = *cursor;
//     char* current = end;
//
//     if (UNLIKELY(current == start)) {
//         lexer_err(lexer, "Invalid output");
//     }
//
//     while (*(current - 1) != '/') {
//         if (current == start) {
//             lexer_err(lexer, "Invalid output");
//         }
//         current--;
//     }
//
//     *cursor = current;
//     (*cursor)--;
//     (**cursor) = 0;
//
//     *cursor = end;
//     (**cursor) = 0;
//     (*cursor)++;
//
//     set_output_name(lexer -> arena, lexer -> config, current);
//     set_output_dir(lexer -> arena, lexer -> config, start);
// }
//
// static void match_options(Lexer* lexer, const char* start) {
//     advance(lexer);
//     skip_whitespace(lexer);
//
//     switch (start[0]) {
//         case 'b':
//             if (strcmp(start, "build_dir") == 0) {
//                 parse_single(lexer, &lexer -> config -> build_dir);
//                 return;
//             }
//             break;
//
//         case 'c':
//             if (strcmp(start, "compiler") == 0) {
//                 parse_single(lexer, &lexer -> config -> compiler); 
//                 return;
//             }
//             break;
//
//         case 'd':
//             if (strcmp(start, "default_flags") == 0) {
//                 parse_default_flags(lexer);
//                 return;
//             }
//             break;
//
//         case 'f':
//             if (strcmp(start, "flags") == 0) {
//                 parse_target_flags(lexer);
//                 return;
//             }
//             break;
//
//         case 'o':
//             if (strcmp(start, "output") == 0) {
//                 parse_target_output(lexer);
//                 return;
//             }
//             break;
//
//         case 's':
//             if (strcmp(start, "sources") == 0) {
//                 parse_sources(lexer);
//                 return;
//             }
//             break;
//     }
//
//     lexer_err(lexer, "Unknown option");
// }
//
// static inline void parse_key(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//     skip_whitespace(lexer);
//
//     if (**cursor == '}') {
//         return;
//     }
//
//     const char* start = *cursor;
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     (**cursor) = 0;
//     (*cursor)++;
//
//     match_options(lexer, start);
// }
//
// static inline void expect_keyword(Lexer* lexer, const char* keyword) {
//     char** cursor = &lexer -> cursor;
//     const char* start = *cursor;
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     size_t len = *cursor - start;
//     if (len != strlen(keyword) || strncmp(start, keyword, len) != 0) {
//         char msg[64];
//         snprintf(msg, sizeof(msg), "Expected: %s", keyword);
//         lexer_err(lexer, msg);
//     }
// }
//
// static inline void expect_char(Lexer* lexer, const char expected) {
//     char** cursor = &lexer -> cursor;
//
//     if (**cursor != expected) {
//         char msg[16];
//         snprintf(msg, sizeof(msg), "Expected: %c", expected);
//         lexer_err(lexer, msg);
//     }
//
//     advance(lexer);
// }
//
// static inline void parse_identifier(Lexer* lexer, char** dest) {
//     char** cursor = &lexer -> cursor;
//     char* start = *cursor;
//
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     (**cursor) = 0;
//     (*cursor)++;
//
//     set_single(lexer -> arena, dest, start);
// }
//
// static inline void parse_config_section(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//
//     expect_keyword(lexer, "config");
//     skip_whitespace(lexer);
//
//     expect_char(lexer, '{');
//
//     skip_whitespace(lexer);
//     while (**cursor != '}') {
//         parse_key(lexer);
//         skip_whitespace(lexer);
//     }
//
//     advance(lexer);
// }
//
// static void parse_target_section(Lexer* lexer) {
//     char** cursor = &lexer -> cursor;
//
//     expect_keyword(lexer, "target");
//     skip_whitespace(lexer);
//
//     char* start = *cursor;
//
//     while (IS_ALPHA(**cursor)) {
//         advance(lexer);
//     }
//
//     (**cursor) = 0;
//     (*cursor)++;
//
//     switch (*start) {
//         case 'e':
//             if (strcmp(start, "executable") == 0) {
//                 set_type(lexer -> arena, lexer -> config, Executable);
//                 break;
//             }
//             lexer_err(lexer, "uknown target type");
//
//         case 't':
//             if (strcmp(start, "test") == 0) {
//                 set_type(lexer -> arena, lexer -> config, Test);
//                 break;
//             }
//             lexer_err(lexer, "uknown target type");
//
//         case 'd':
//             if (strcmp(start, "debug") == 0) {
//                 set_type(lexer -> arena, lexer -> config, Debug);
//                 break;
//             }
//             lexer_err(lexer, "uknown target type");
//
//         case 's':
//             if (strcmp(start, "static_lib") == 0) {
//                 set_type(lexer -> arena, lexer -> config, StaticLib);
//                 break;
//             } else if (strcmp(start, "shared_lib") == 0) {
//                 set_type(lexer -> arena, lexer -> config, SharedLib);
//                 break;
//             }
//             lexer_err(lexer, "uknown target type");
//
//         default:
//             lexer_err(lexer, "Uknown target type");
//     }
//
//     skip_whitespace(lexer);
//
//     Target* target = lexer -> config -> targets[lexer -> config -> target_count];
//     parse_identifier(lexer, &target -> name);
//
//     skip_whitespace(lexer);
//     expect_char(lexer, '{');
//     skip_whitespace(lexer);
//
//     while (**cursor != '}') {
//         parse_key(lexer);
//         skip_whitespace(lexer);
//     }
//
//     advance(lexer);
//     lexer -> config -> target_count++;
// }
//
// CatalyzeConfig* lexer_parse(ArenaAllocator* arena, char* buffer, const size_t size, const uint8_t nest_count) {
//     CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
//     arena_memset(config, 0, sizeof(*config));
//
//     Lexer* lexer = create_lexer(arena, config, buffer, size);
//
//     config -> target_count = 0;
//     config -> default_flag_count = 0;
//     config -> nest_count = nest_count;
//
//     skip_whitespace(lexer);
//     parse_config_section(lexer);
//
//     char** cursor = &lexer -> cursor;
//     skip_whitespace(lexer);
//
//     while (**cursor == 't' && lexer -> config -> target_count < MAX_TARGETS) {
//         parse_target_section(lexer);
//         skip_whitespace(lexer);
//     }
//
//     if (UNLIKELY(lexer -> config -> target_count == 0)) {
//         lexer_err(lexer, "At least one target is required");
//     }
//
//     return lexer -> config;
// }
