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

Result find_config_file(Arena* arena, uint8_t* nest_count) {
    char* curr_path = arena_alloc(arena, MAX_PATH);
    getcwd(curr_path, MAX_PATH);

    // little micro optimisation here, saved some nanoseconds :3 
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

Result parse_config(Arena* arena) { 
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
        return err("Failed to get stats about config.cat");
    }

    char* data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
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
    printf("=== CatalyzeConfig ===\n");
    printf("Compiler: %s\n", config -> compiler);
    printf("Build dir: %s\n", config -> build_dir);
    
    printf("Default flags (%d):\n", config->default_flag_count);
    for (int i = 0; i < config -> default_flag_count; i++) {
        printf("  - %s\n", config -> default_flags[i]);
    }
    
    printf("Targets (%d):\n", config->target_count);
    for (int i = 0; i < config -> target_count; i++) {
        print_target(&config -> targets[i]);
    }
    printf("======================\n");
}
