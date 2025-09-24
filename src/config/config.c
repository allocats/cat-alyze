#include "config.h"
#include "lexer.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

Result push_default_flag(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, push_default_flag");
    }

    if (*len >= MAX_FLAG_LEN) {
        return err("Flag too long");
    }

    if (config -> default_flag_count >= MAX_FLAGS) {
        return err("Defulat flags at max capacity");
    }

    if (config -> default_flags == NULL) {
        config -> default_flags = arena_alloc(arena, MAX_FLAGS * sizeof(char*));
        arena_memset(config -> default_flags, 0, *len);
    }

    config -> default_flags[config -> default_flag_count] = arena_alloc(arena, *len + 1);
    memcpy(config -> default_flags[config -> default_flag_count], start, *len);
    config -> default_flags[config -> default_flag_count][*len] = '\0';
    config -> default_flag_count++;

    return ok(NULL);

}

Result push_flag(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, push_flag");
    }

    if (*len >= MAX_FLAG_LEN) {
        return err("Flag too long");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> flag_count >= MAX_FLAGS) {
        return err("Target flags at max capacity");
    }

    if (target -> flags == NULL) {
        target -> flags = arena_alloc(arena, MAX_FLAGS * (sizeof(char*)));
        arena_memset(target -> flags, 0, *len);
    }

    target -> flags[target -> flag_count] = arena_alloc(arena, *len + 1);
    memcpy(target -> flags[target -> flag_count], start, *len);
    target -> flags[target -> flag_count][*len] = '\0';
    target -> flag_count++;

    return ok(NULL);
}

Result push_source(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, push_source");
    }

    if (*len >= MAX_SOURCE_LEN) {
        return err("Source path too long");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> source_count >= MAX_SOURCES) {
        return err("Target sources at max capacity");
    }

    if (target -> sources == NULL) {
        target -> sources = arena_alloc(arena, MAX_SOURCES * (sizeof(char*)));
        arena_memset(target -> sources, 0, *len);
    }

    target -> sources[target -> source_count] = arena_alloc(arena, *len + 1);
    memcpy(target -> sources[target -> source_count], start, *len);
    target -> sources[target -> source_count][*len] = '\0';
    target -> source_count++;

    return ok(NULL);
}

Result set_single(ArenaAllocator* arena, char** dest, const char* start, size_t len) {
    if (len >= MAX_NAME_LEN) {
        return err("Field too long");
    }

    if (*dest == NULL) {
        *dest = arena_alloc(arena, len + 1);
        arena_memset((void*) *dest, 0, len + 1);
    }

    memcpy((void*) *dest, start, len);
    (*dest)[len] = '\0';
    return ok(NULL);
}

Result set_compiler(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, set_compiler");
    }

    if (*len >= MAX_COMPILER_LEN) {
        return err("Compiler too long");
    }

    if (config -> compiler == NULL) {
        config -> compiler = arena_alloc(arena, *len + 1);
        arena_memset(config -> default_flags, 0, *len);
    }

    memcpy(config -> compiler, start, *len);
    config -> compiler[*len] = '\0';

    return ok(NULL);
}

Result set_build_dir(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, set_build_dir");
    }

    if (*len >= MAX_BUILD_DIR_LEN) {
        return err("Build directory path too long");
    }

    if (config -> build_dir == NULL) {
        config -> build_dir = arena_alloc(arena, *len + 1);
        arena_memset(config -> default_flags, 0, *len);
    }

    memcpy(config -> build_dir, start, *len);
    config -> build_dir[*len] = '\0';

    return ok(NULL);
}

Result set_name(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, set_name");
    }

    Target* target = config -> targets[config -> target_count];
    if (target == NULL) {
        target = arena_alloc(arena, sizeof(*target));
        arena_memset(target, 0, sizeof(*target));

        target -> source_count = 0;
        target -> flag_count = 0;
    }

    if (target -> name == NULL) {
        target -> name = arena_alloc(arena, *len + 1);
        arena_memset(target -> name, 0, *len);
    }

    memcpy(target -> name, start, *len);
    target -> name[*len] = '\0';

    return ok(NULL);
}

Result set_type(ArenaAllocator* arena, CatalyzeConfig* config, TargetType type) {
    if (config == NULL) {
        return err("Null value, set_type");
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

    return ok(NULL);
}

Result set_output_dir(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, set_output_dir");
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

    memcpy(target -> output_dir, start, *len);
    target -> output_dir[*len] = '\0';


    return ok(NULL);
}

Result set_output_name(ArenaAllocator* arena, CatalyzeConfig* config, const char* start, size_t* len) {
    if (config == NULL || start == NULL || len == NULL) {
        return err("Null value, set_output_name");
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

    memcpy(target -> output_name, start, *len);
    target -> output_name[*len] = '\0';

    return ok(NULL);
}


Result find_config_file(ArenaAllocator* arena, uint8_t* nest_count) {
    char* curr_path = arena_alloc(arena, MAX_PATH);
    getcwd(curr_path, MAX_PATH);

    for (int i = 0; i < MAX_PATH >> 2; i++) {
        DIR* dir = opendir(curr_path);
        if (!dir) {
            return err("Failed to open directory\n");
        };

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry -> d_type == DT_REG && strcmp("config.cat", entry -> d_name) == 0) {
                char* result_path = arena_alloc(arena, MAX_PATH);
                snprintf(result_path, MAX_PATH, "%s/config.cat", curr_path);
                closedir(dir);

                return ok(result_path);
            }
        }
        closedir(dir);
        strcat(curr_path, "/..");

        *nest_count = i + 1;
    }

    return err("config.cat not found");
}

Result parse_config(ArenaAllocator* arena) { 
    uint8_t nest_count = 0;
    Result path = find_config_file(arena, &nest_count);

    if (IS_ERR(path)) {
        return err(ERR_MSG(path));
    } 

    int fd = open(path.data, O_RDONLY);
    if (fd == -1) {
        return err("Failed to open config.cat");
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        close(fd);
        return err("Failed to get stats about config.cat");
    }

    char* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        close(fd);
        return err("Failed to allocate buffer for config.cat");
    }

    char* buffer = arena_alloc(arena, st.st_size + 1);
    arena_memcpy(buffer, data, st.st_size);
    munmap(data, st.st_size);
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
