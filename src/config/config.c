#include "config.h"
#include "lexer.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static inline void config_err(const char* msg) {
    printf("\e[1mError in config.cat:\e[0m %s\n", msg);
    exit(1);
}

void push_default_flag(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
        config_err("Null value, push_default_flag");
    }

    size_t flag_count = config -> default_flag_count;

    if (config -> default_flags == NULL) {
        config -> default_flags = arena_alloc(arena, 16 * sizeof(char*));
    }

    config -> default_flags[flag_count++] = start;
    config -> default_flag_count++;
}

void push_flag(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
        config_err("Null value, push_flag");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> flag_count >= MAX_FLAGS) {
        config_err("Target flags at max capacity");
    }

    if (target -> flags == NULL) {
        target -> flags = arena_alloc(arena, 16 * (sizeof(char*)));
    }

    target -> flags[target -> flag_count] = start;
    target -> flag_count++;
}

void push_source(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
        config_err("Null value, push_source");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> source_count >= MAX_SOURCES) {
        config_err("Target sources at max capacity");
    }

    if (target -> sources == NULL) {
        target -> sources = arena_alloc(arena, 32 * (sizeof(char*)));
    }

    target -> sources[target -> source_count] = start;
    target -> source_count++;
}

void set_single(ArenaAllocator* arena, char** dest, char* start) {
    if (*dest == NULL) {
        *dest = arena_alloc(arena, sizeof(char*));
    }

    *dest = start;
}

void set_compiler(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
        config_err("Null value, set_compiler");
    }

    if (config -> compiler == NULL) {
        config -> compiler = arena_alloc(arena, sizeof(char*));
    }

    config -> compiler = start;
}

void set_build_dir(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
        config_err("Null value, set_build_dir");
    }

    if (config -> build_dir == NULL) {
        config -> build_dir = arena_alloc(arena, sizeof(char*));
    }

    config -> build_dir = start;
}

void set_name(ArenaAllocator* arena, CatalyzeConfig* config, char* start) {
    if (config == NULL || start == NULL) {
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

void set_type(ArenaAllocator* arena, CatalyzeConfig* config, TargetType type) {
    if (config == NULL) {
        config_err("Null value, set_type");
    }

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

void set_output_dir(ArenaAllocator* arena, CatalyzeConfig* config, char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        config_err("Null value, set_output_dir");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));
    }

    if (target -> output_dir == NULL) {
        target -> output_dir = arena_alloc(arena, *len + 1);
        arena_memset(target -> output_dir, 0, *len);
    }

    arena_memcpy(target -> output_dir, start, *len);
    target -> output_dir[*len] = '\0';
}

void set_output_name(ArenaAllocator* arena, CatalyzeConfig* config, char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        config_err("Null value, set_output_name");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));
    }

    if (target -> output_name == NULL) {
        target -> output_name = arena_alloc(arena, *len + 1);
        arena_memset(target -> output_name, 0, *len);
    }

    arena_memcpy(target -> output_name, start, *len);
    target -> output_name[*len] = '\0';
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
            if (!result_path) {
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
    if (fd == -1) {
        config_err("Failed to open config.cat");
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        config_err("Failed to get stats about config.cat");
    }

    char* buffer = arena_alloc(arena, st.st_size + 1);
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (bytes_read != st.st_size) {
        close(fd);
        config_err("Failed to read config.cat");
    }

    close(fd);
    buffer[st.st_size] = '\0';

    return lexer_parse(arena, buffer, nest_count);
}

static const char* target_type_to_string(TargetType type) {
    switch (type) {
        case Executable: return "executable";
        case Debug:      return "debug";
        case Test:       return "test";
        case StaticLib:  return "staticlib";
        case SharedLib:  return "sharedlib";
        default:         return "unknown";
    }
}

static void print_target(const Target* target) {
    printf("  Target '%s' (%s):\n", target -> name, target_type_to_string(target -> type));
    
    printf("    Sources (%d):\n", target -> source_count);
    for (int i = 0; i < target ->source_count; i++) {
        printf("      - %s\n", target -> sources[i]);
    }
    
    printf("    Flags (%d):\n", target -> flag_count);
    for (int i = 0; i < target -> flag_count; i++) {
        printf("      - %s\n", target -> flags[i]);
    }
    
    printf("    Output dir: %s\n", target -> output_dir);
    printf("    Output name: %s\n", target -> output_name);
    printf("\n");
}

void print_config(const CatalyzeConfig* config) {
    printf("=== CatalyzeConfig ===\n\n");
    printf("Compiler: %s\n", config -> compiler);
    printf("Build dir: %s\n", config -> build_dir);
    
    printf("Default flags (%d):\n", config->default_flag_count);
    for (int i = 0; i < config -> default_flag_count; i++) {
        printf("  - %s\n", config -> default_flags[i]);
    }
    
    printf("Targets (%d):\n", config->target_count);
    for (int i = 0; i < config -> target_count; i++) {
        print_target(config -> targets[i]);
    }

    printf("======================\n");
}
