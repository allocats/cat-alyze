#include "lexer.h" 
#include "config.h"

#include <dirent.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

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
    ['*'] = 1,

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

static void skip_spaces(Lexer* lexer) {
    while (IS_WHITESPACE(lexer -> c) && lexer -> c != '\n') {
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

static Result parse_default_flags(Lexer* lexer) {
    uint8_t i = 0;

    while (lexer -> c != '\n' && i < MAX_FLAGS) {
        skip_spaces(lexer);
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

        strncpy(lexer -> config -> default_flags[i], flag_start, flag_len);
        lexer -> config -> default_flags[i][flag_len] = '\0';
        i++;
    }

    lexer -> config -> default_flag_count = i;
    return ok(NULL);
}

static Result parse_target_flags(Lexer* lexer) {
    uint8_t i = lexer -> config -> target_count;
    uint8_t count = 0;

    while (lexer -> c != '\n' && count < MAX_FLAGS) {
        skip_spaces(lexer);
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

        strncpy(lexer -> config -> targets[i].flags[count], flag_start, flag_len);
        lexer -> config -> targets[i].flags[count][flag_len] = '\0';
        count++;
    }

    lexer -> config -> targets[i].flag_count = count;
    return ok(NULL);
}

static Result parse_sources(Lexer* lexer) {
    uint8_t i = lexer -> config -> target_count;
    uint8_t count = 0;

    while (lexer -> c != '\n' && count < MAX_SOURCES) {
        skip_spaces(lexer);
        if (lexer -> c == '\n') break;

        const char* source_start = &lexer -> buffer[lexer -> current];
        while (!IS_WHITESPACE(lexer -> c) && lexer -> c != '\n') {
            advance(lexer);
        }

        size_t source_len = &lexer -> buffer[lexer -> current] - source_start;
        if (source_len > MAX_SOURCE_LEN) {
            lexer_err(lexer, "Invalid source");
            return err("Invalid source");
        }

        strncpy(lexer -> config -> targets[i].sources[count], source_start, source_len);
        lexer -> config -> targets[i].sources[count][source_len] = '\0';
        count++;
    }

    lexer -> config -> targets[i].source_count = count;
    return ok(NULL);
}

static Result parse_target_output(Lexer* lexer) {
    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];

    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    const char* end = &lexer -> buffer[lexer -> current];
    const char* current = end;

    if (current == start) {
        lexer_err(lexer, "Invalid output");
        return err("Invalid output path");
    }

    while (*(current - 1) != '/') {
        if (current == start) {
            lexer_err(lexer, "Invalid output");
            return err("Invalid output path");
        }
        current--;
    }

    size_t len = end - current;
    strncpy(target -> output_name, current, len);
    target -> output_name[len] = '\0';

    len = current - start;
    strncpy(target -> output_dir, start, len);
    target -> output_dir[len] = '\0';

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

        case 'd':
            if (strncmp(start, "default_flags", len) == 0) return parse_default_flags(lexer);
            break;

        case 'f':
            if (strncmp(start, "flags", len) == 0) return parse_target_flags(lexer);
            break;

        case 'o':
            if (strncmp(start, "output", len) == 0) return parse_target_output(lexer);
            break;

        case 's':
            if (strncmp(start, "sources", len) == 0) return parse_sources(lexer);
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

static Result expect_keyword(Lexer* lexer, const char* keyword) {
    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer -> c)) {
        advance(lexer);
    }

    size_t len = &lexer -> buffer[lexer -> current] - start;
    if (len != strlen(keyword) || strncmp(start, keyword, len) != 0) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Expected: %s", keyword);
        lexer_err(lexer, msg);
        return err("Parsing failed");
    }

    return ok(NULL);
}

static Result expect_char(Lexer* lexer, const char expected) {
    if (lexer -> c != expected) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Expected: %c", expected);
        lexer_err(lexer, msg);
        return err("Parsing failed");
    }

    advance(lexer);
    return ok(NULL);
}

static Result parse_identifier(Lexer* lexer, char* dest, size_t max_len) {
    const char* start = &lexer -> buffer[lexer -> current];
    while (IS_ALPHA(lexer ->c)) {
        advance(lexer);
    }

    size_t len = &lexer -> buffer[lexer -> current] - start;
    if (len >= max_len) {
        lexer_err(lexer, "Identifier too long");
        return err("Parsing failed");
    }

    strncpy(dest, start, len);
    dest[len] = '\0';
    return ok(NULL);
}

static Result parse_config_section(Lexer* lexer) {
    Result result;

    result = expect_keyword(lexer, "config");
    if (IS_ERR(result)) return result;

    skip_whitespace(lexer);

    result = expect_char(lexer, '{');
    if (IS_ERR(result)) return result;

    skip_whitespace(lexer);
    while (lexer -> c != '}') {
        result = parse_key(lexer);
        if (IS_ERR(result)) return result;
        skip_whitespace(lexer);
    }

    advance(lexer);
    return ok(NULL);
}

static Result parse_target_section(Lexer* lexer) {
    Result result;

    result = expect_keyword(lexer, "target");
    if (IS_ERR(result)) return result;


    skip_whitespace(lexer);

    if (IS_ALPHA(lexer -> c)) {
        char type[32];
        result = parse_identifier(lexer, type, sizeof(type));
        if (IS_ERR(result)) return result;

        switch (type[0]) {
            case 'e':
                if (strcmp(type, "executable") == 0) lexer -> config -> targets[lexer -> config -> target_count].type = Executable;
                break;

            case 't':
                if (strcmp(type, "test") == 0) lexer -> config -> targets[lexer -> config -> target_count].type = Test;
                break;

            case 'd':
                if (strcmp(type, "debug") == 0) lexer -> config -> targets[lexer -> config -> target_count].type = Debug;
                break;

            case 's':
                if (strcmp(type, "static_lib") == 0) lexer -> config -> targets[lexer -> config -> target_count].type = StaticLib;
                if (strcmp(type, "shared_lib") == 0) lexer -> config -> targets[lexer -> config -> target_count].type = SharedLib;
                break;

            default:
                lexer_err(lexer, "Uknown target type");
                return err("Uknown target type");
        }
    }

    skip_whitespace(lexer);

    Target* target = &lexer -> config -> targets[lexer -> config -> target_count];
    result = parse_identifier(lexer, target -> name, MAX_NAME_LEN);
    if (IS_ERR(result)) return result;

    skip_whitespace(lexer);

    result = expect_char(lexer, '{');
    if (IS_ERR(result)) return result;

    skip_whitespace(lexer);

    while (lexer -> c != '}') {
        result = parse_key(lexer);
        if (IS_ERR(result)) return result;
        skip_whitespace(lexer);
    }

    advance(lexer);
    lexer -> config -> target_count++;

    return ok(NULL);
}

Result lexer_parse(Arena* arena, const char* buffer, uint8_t nest_count) {
    CatalyzeConfig* config = arena_alloc(arena, sizeof(*config));
    arena_memset(config, 0, sizeof(*config));

    Lexer* lexer = create_lexer(arena, config, buffer);
    Result result;

    config -> target_count = 0;
    config -> nest_count = nest_count;

    skip_whitespace(lexer);
    result = parse_config_section(lexer);
    if (IS_ERR(result)) return result;

    skip_whitespace(lexer);
    while (lexer -> c == 't' && lexer -> config -> target_count < MAX_TARGETS) {
        result = parse_target_section(lexer);
        if (IS_ERR(result)) return result;
        skip_whitespace(lexer);
    }

    if (lexer -> config -> target_count == 0) {
        lexer_err(lexer, "At least on target is required");
        return err("No targets found");
    }

    return ok(lexer -> config);
}
