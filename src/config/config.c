#include "config.h"

#include "lexer.h"
#include "../utils/macros.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void config_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
    exit(1);
}

inline void push_default_flag(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, push_default_flag");
    }

    size_t flag_count = config -> default_flag_count;

    if (config -> default_flags == NULL) {
        config -> default_flags = arena_alloc(arena, 16 * sizeof(char*));
    }

    config -> default_flags[flag_count++] = start;
    config -> default_flag_count++;
}

inline void push_flag(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, push_flag");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (UNLIKELY(target -> flag_count >= MAX_FLAGS)) {
        config_err("Target flags at max capacity");
    }

    if (target -> flags == NULL) {
        target -> flags = arena_alloc(arena, 16 * (sizeof(char*)));
    }

    target -> flags[target -> flag_count] = start;
    target -> flag_count++;
}

inline void push_source(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, push_source");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (UNLIKELY(target -> source_count >= MAX_SOURCES)) {
        config_err("Target sources at max capacity");
    }

    if (target -> sources == NULL) {
        target -> sources = arena_alloc(arena, 32 * (sizeof(char*)));
    }

    target -> sources[target -> source_count] = start;
    target -> source_count++;
}

inline void set_single(ArenaAllocator* arena, char** dest, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_single");
    }

    if (*dest == NULL) {
        *dest = arena_alloc(arena, sizeof(char*));
    }

    *dest = start;
}

inline void set_compiler(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_compiler");
    }

    if (config -> compiler == NULL) {
        config -> compiler = arena_alloc(arena, sizeof(char*));
    }

    config -> compiler = start;
}

inline void set_build_dir(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_build_dir");
    }

    if (config -> build_dir == NULL) {
        config -> build_dir = arena_alloc(arena, sizeof(char*));
    }

    config -> build_dir = start;
}

inline void set_name(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_name");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> name == NULL) {
        target -> name = arena_alloc(arena, sizeof(char*));
    }

    target -> name = start;
}

inline void set_type(ArenaAllocator* arena, CatalyzeConfig* config, TargetType type) {
    if (config -> targets == NULL) {
        config -> targets = arena_alloc(arena, MAX_TARGETS * sizeof(Target*));
        arena_memset(config -> targets, 0, MAX_TARGETS * sizeof(Target*));
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));
        config -> targets[config -> target_count] = target;
    }

    target -> type = type;
}

inline void set_output_dir(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_output_dir");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));
    }

    if (target -> output_dir == NULL) {
        target -> output_dir = arena_alloc(arena, sizeof(char*));
    }

    target -> output_dir = start;
}

inline void set_output_name(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (UNLIKELY(start == NULL)) {
        config_err("Null value, set_output_name");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));
    }

    if (target -> output_name == NULL) {
        target -> output_name = arena_alloc(arena, sizeof(char*));
    }

    target -> output_name = start;
}

static char* find_config_file(ArenaAllocator* arena, uint8_t* nest_count) {
    static const char* path_templates[] = {
        "./config.cat",
        "../config.cat", 
        "../../config.cat",
        "../../../config.cat",
        "../../../../config.cat",
        "../../../../../config.cat",
        "../../../../../../config.cat",
        "../../../../../../../config.cat",
        "../../../../../../../../config.cat",
        "../../../../../../../../../config.cat",
        "../../../../../../../../../../config.cat",
        "../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../../../config.cat",
        NULL
    };
    
    for (int i = 0; path_templates[i]; i++) {
        FILE* fptr = fopen(path_templates[i], "r");
        if (fptr) {
            fclose(fptr);
            *nest_count = i;
            
            size_t path_len = strlen(path_templates[i]);
            char* result_path = arena_alloc(arena, path_len + 1);
            if (UNLIKELY(!result_path)) {
                config_err("Arena allocation failed");
            }

            strcpy(result_path, path_templates[i]);
            return result_path;
        }
    }
    
    config_err("config.cat not found");
    exit(1);
}

CatalyzeConfig* parse_config(ArenaAllocator* arena) { 
    uint8_t nest_count = 0;
    char* path = find_config_file(arena, &nest_count);

    int fd = open(path, O_RDONLY);
    if (UNLIKELY(fd == -1)) {
        config_err("Failed to open config.cat");
    }

    struct stat st;
    int result = fstat(fd, &st);
    if (UNLIKELY(result == -1)) {
        close(fd);
        config_err("Failed to get stats about config.cat");
    }

    char* buffer = arena_alloc(arena, st.st_size + 1);
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (UNLIKELY(bytes_read != st.st_size)) {
        close(fd);
        config_err("Failed to read config.cat");
    }

    close(fd);
    buffer[st.st_size] = '\0';

    return lexer_parse(arena, buffer, st.st_size, nest_count);
}
