#include "build.h"

#include "../utils/macros.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_THREADS sysconf(_SC_NPROCESSORS_ONLN)

static inline void build_err(const char* msg) {
    printf("\e[1mError:\e[0m %s\n", msg);
    exit(1);
}

inline void make_dir(const char* dir) {
    char tmp[PATH_MAX];
    char *p = NULL;

    snprintf(tmp, sizeof(tmp), "%s", dir);
    size_t len = strlen(tmp);

    if (tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(tmp, 0755) != 0) {
                if (errno != EEXIST) {
                    build_err("Mkdir failed!");
                }
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, 0755) != 0) { 
        if (errno != EEXIST) {
            build_err("Mkdir failed!");
        }
    }
}

inline char* source_to_object_name(ArenaAllocator* arena, const char* source_path) {
    const char* name = strrchr(source_path, '/');
    name = name ? name + 1 : source_path;

    const char* ext = strrchr(name, '.');
    size_t len = ext ? (size_t)(ext - name) : strlen(name);

    char* object_name = arena_alloc(arena, len + 3);
    strncpy(object_name, name, len);
    strcpy(object_name + len, ".o");

    return object_name;
}

void link_executable(CatalyzeConfig* config, const char* path_prefix, Target* build_target, char** all_flags, uint8_t flag_count, char** all_object_files) {
    size_t size = 32 + strlen(config -> compiler) + 1; 
    size += strlen(path_prefix) + 1;
    size += strlen(build_target -> output_dir) + 1;
    size += strlen(build_target -> output_name) + 1;
    
    for (uint8_t i = 0; i < flag_count; i++) {
        size += strlen(all_flags[i]) + 1;
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        size += strlen(all_object_files[i]) * 2;
        size += strlen(path_prefix) + 1;
    }

    size += strlen(path_prefix) + 1;

    char cmd[size];
    size_t offset = snprintf(cmd, size, "%s", config -> compiler);

    for (uint8_t i = 0; i < flag_count; i++) {
        offset += snprintf(cmd + offset, size - offset, " %s", all_flags[i]);
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        offset += snprintf(cmd + offset, size - offset, " %s%s%s", path_prefix, config -> build_dir, all_object_files[i]);
    }

    offset += snprintf(cmd + offset, size - offset, " -o");
    offset += snprintf(cmd + offset, size - offset, " %s%s/%s", path_prefix, build_target -> output_dir, build_target -> output_name);

    if (UNLIKELY(system(cmd) != 0)) {
        build_err("compiler failed");
    }
}

static void* compile_object(void* arg) {
    Cmd* cmd = (Cmd*) arg;

    size_t size = 32 + strlen(cmd -> compiler) + 1;
    for (int i = 0; i < cmd -> flag_count; i++) {
        size += strlen(cmd -> all_flags[i]) + 1;
    }

    char* object_file = source_to_object_name(cmd -> arena, cmd -> source);

    size += strlen(cmd -> source) + 1;
    size += strlen(cmd -> path_prefix) + 1;
    size += strlen(cmd -> build_dir) + 1;
    size += strlen(object_file) + 1;
    size += strlen(cmd -> path_prefix) + 1;

    size = align_size(size);
    char compile_command[size];

    size_t offset = snprintf(compile_command, size, "%s", cmd -> compiler);

    for (uint8_t i = 0; i < cmd -> flag_count; i++) {
        offset += snprintf(compile_command + offset, size - offset, " %s", cmd -> all_flags[i]); 
    }

    offset += snprintf(compile_command + offset, size - offset, " -c");
    offset += snprintf(compile_command + offset, size - offset, " %s%s", cmd -> path_prefix, cmd -> source);

    offset += snprintf(compile_command + offset, size - offset, " -o");
    offset += snprintf(compile_command + offset, size - offset, " %s%s%s", cmd -> path_prefix, cmd -> build_dir, object_file);

    (*cmd -> all_object_files)[cmd -> idx] = arena_strdup(cmd -> arena, object_file);

    if (UNLIKELY(system(compile_command) != 0)) {
        exit(1);
    }

    return NULL;
}

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target) {
    make_dir(config -> build_dir);

    Target* build_target = NULL;
    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(target, config -> targets[i] -> name) == 0) {
            build_target = config -> targets[i];
            break;
        }
    }

    if (UNLIKELY(build_target == NULL)) {
        build_err("Target not found");
    }

    make_dir(build_target -> output_dir);

    char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    char** all_flags = arena_array(arena, char*, (config -> default_flag_count + build_target -> flag_count));
    uint8_t flag_count = config -> default_flag_count + build_target -> flag_count;

    if (config -> default_flag_count > 0) {
        arena_memcpy(all_flags, config -> default_flags, config -> default_flag_count * sizeof(char*));
    }

    if (build_target -> flag_count > 0) {
        arena_memcpy(all_flags + config -> default_flag_count, build_target -> flags, build_target-> flag_count * sizeof(char*));
    }

    char** all_object_files = arena_array(arena, char*, build_target -> source_count);

    int idx = 0;
    pthread_t threads[MAX_THREADS];

    while (idx < build_target -> source_count) {
        int batch_size = (build_target->source_count - idx > MAX_THREADS) ? MAX_THREADS : (build_target->source_count - idx);

        for (int i = 0; i < batch_size; i++) {
            Cmd* cmd = arena_alloc(arena, sizeof(*cmd));

            cmd -> all_object_files = &all_object_files;
            cmd -> all_flags = all_flags;
            cmd -> flag_count = flag_count;
            cmd -> compiler = config -> compiler;
            cmd -> arena = arena;
            cmd -> source = build_target -> sources[idx];
            cmd -> build_dir = config -> build_dir;
            cmd -> path_prefix = path_prefix;
            cmd -> idx = idx;

            pthread_create(&threads[i], NULL, compile_object, cmd);
            idx++;
        }

        for (int i = 0; i < batch_size; i++) {
            pthread_join(threads[i], NULL);
        }
    }

    switch (build_target -> type) {
        case Executable:
        case Debug:
        case Test:
            link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
            break;

        default:
            build_err("Unknown target");
    }
}

void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    make_dir(config -> build_dir);

    for (uint8_t current_target = 0; current_target < config -> target_count; current_target++) {
        Target* build_target = config -> targets[current_target];

        if (build_target == NULL) {
            build_err("Target not found");
        }

        if (build_target -> type != Executable) continue;

        make_dir(build_target -> output_dir);

        char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
        path_prefix[0] = '\0';

        for (uint8_t i = 0; i < config -> nest_count; i++) {
            strcat(path_prefix, "../");
        }

        char** all_flags = arena_array(arena, char*, (config -> default_flag_count + build_target -> flag_count));
        uint8_t flag_count = config -> default_flag_count + build_target -> flag_count;

        if (config -> default_flag_count > 0) {
            arena_memcpy(all_flags, config -> default_flags, config -> default_flag_count * sizeof(char*));
        }

        if (build_target -> flag_count > 0) {
            arena_memcpy(all_flags + config -> default_flag_count, build_target -> flags, build_target-> flag_count * sizeof(char*));
        }

        char** all_object_files = arena_array(arena, char*, build_target -> source_count);

        int idx = 0;
        pthread_t threads[MAX_THREADS];

        while (idx < build_target -> source_count) {
            int batch_size = (build_target->source_count - idx > MAX_THREADS) ? MAX_THREADS : (build_target->source_count - idx);

            for (int i = 0; i < batch_size; i++) {
                Cmd* cmd = arena_alloc(arena, sizeof(*cmd));

                cmd -> all_object_files = &all_object_files;
                cmd -> all_flags = all_flags;
                cmd -> flag_count = flag_count;
                cmd -> compiler = config -> compiler;
                cmd -> arena = arena;
                cmd -> source = build_target -> sources[idx];
                cmd -> build_dir = config -> build_dir;
                cmd -> path_prefix = path_prefix;
                cmd -> idx = idx;

                pthread_create(&threads[i], NULL, compile_object, cmd);
                idx++;
            }

            for (int i = 0; i < batch_size; i++) {
                pthread_join(threads[i], NULL);
            }
        }

        switch (build_target -> type) {
            case Executable:
            case Debug:
            case Test:
                link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
                break;

            default:
                build_err("Unknown target");
        }
    }
}
