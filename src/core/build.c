#include "build.h"

#include "../utils/macros.h"

#define WHISKER_NOPREFIX
#include "../../whisker/cmd/whisker_cmd.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_THREADS sysconf(_SC_NPROCESSORS_ONLN)

void build_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
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

inline const char* source_to_object_name(ArenaAllocator* arena, const char* source_path) {
    const char* name = strrchr(source_path, '/');
    name = name ? name + 1 : source_path;

    const char* ext = strrchr(name, '.');
    const size_t len = (size_t)(ext - name);

    char* object_name = arena_alloc(arena, len + 3);
    strncpy(object_name, name, len);
    strcpy(object_name + len, ".o");

    return object_name;
}

void link_executable(ArenaAllocator* arena, CatalyzeConfig* config, const char* path_prefix, Target* build_target, char** all_flags, uint8_t flag_count, char** all_object_files) {
    Whisker_Cmd cmd = {0};

    cmd_append(&cmd, config -> compiler);

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        const size_t size = 16 + strlen(path_prefix) + strlen(config -> build_dir) + strlen(all_object_files[i]); 
        char* obj_paths = arena_alloc(arena, size);
        snprintf(obj_paths, size, "%s%s%s", path_prefix, config -> build_dir, all_object_files[i]);

        cmd_append(&cmd, obj_paths);
    }

    cmd_append(&cmd, "-o");

    const size_t size = 16 + strlen(path_prefix) + strlen(build_target -> output_dir) + strlen(build_target -> output_name);
    char temp[size];
    snprintf(temp, size, "%s%s/%s", path_prefix, build_target -> output_dir, build_target -> output_name);

    cmd_append(&cmd, temp);

    for (uint8_t i = 0; i < flag_count; i++) {
        cmd_append(&cmd, all_flags[i]);
    }

    if (!cmd_execute(&cmd)) {
        build_err("Failed to compile");
    }

#ifdef DEBUG_MODE
    cmd_print(&cmd);
#endif

    cmd_destroy(&cmd);
}

static void* compile_object(void* args) {
    Arg* arg = (Arg*) args;
    Whisker_Cmd cmd = {0};

    const char* object_file = source_to_object_name(arg -> arena, arg -> source);

    cmd_append(&cmd, arg -> compiler);
    cmd_append(&cmd, "-c");

    size_t size = 16 + strlen(arg -> path_prefix) + strlen(arg -> source);
    char source[size];
    snprintf(source, size, "%s%s", arg -> path_prefix, arg -> source);

    cmd_append(&cmd, source);
    cmd_append(&cmd, "-o");

    size = 16 + strlen(arg -> path_prefix) + strlen(arg -> build_dir) + strlen(object_file);
    char build[size];
    snprintf(build, size, "%s%s%s", arg -> path_prefix, arg -> build_dir, object_file);

    cmd_append(&cmd, build);

    for (uint8_t i = 0; i < arg -> flag_count; i++) {
        cmd_append(&cmd, arg -> all_flags[i]);
    }

    (*arg -> all_object_files)[arg -> idx] = arena_strdup(arg -> arena, object_file);

    if (!cmd_execute(&cmd)) {
        build_err("Failed to compile");
    }

#ifdef DEBUG_MODE
    cmd_print(&cmd);
#endif

    cmd_destroy(&cmd);
    return NULL;
}

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target) {
    make_dir(config -> build_dir);

    Target* build_target = NULL;
    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(target, config -> targets[i].name) == 0) {
            build_target = &config -> targets[i];
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

    size_t default_flags_size = sizeof(char*) * config -> default_flag_count;
    size_t target_flags_size = sizeof(char*) * build_target -> flag_count;
    size_t all_flag_count = build_target -> flag_count + config -> default_flag_count;

    char** all_flags = arena_alloc(arena, default_flags_size + target_flags_size);
    arena_memcpy(all_flags, config -> default_flags, default_flags_size);
    arena_memcpy(all_flags + config -> default_flag_count, build_target -> flags, target_flags_size);

#ifdef DEBUG_MODE
    printf("DEBUG: Default flags (%d:%zuB)\n", config -> default_flag_count, default_flags_size);
    for (uint8_t i = 0; i < config -> default_flag_count; i++) {
        printf("%s\n", config -> default_flags[i]);
    }

    printf("\nDEBUG: Target flags (%d:%zuB)\n", build_target -> flag_count, target_flags_size);
    for (uint8_t i = 0; i < build_target -> flag_count; i++) {
        printf("%s\n", build_target -> flags[i]);
    }

    printf("\n");
#endif /* ifdef DEBUG_MODE */

    char** all_object_files = arena_array(arena, char*, build_target -> source_count);

    int idx = 0;
    pthread_t threads[MAX_THREADS];

    while (idx < build_target -> source_count) {
        int batch_size = (build_target->source_count - idx > MAX_THREADS) ? MAX_THREADS : (build_target->source_count - idx);

        for (int i = 0; i < batch_size; i++) {
            Arg* arg = arena_alloc(arena, sizeof(*arg));

            arg -> all_object_files = &all_object_files;
            arg -> all_flags = all_flags;
            arg -> flag_count = all_flag_count;
            arg -> compiler = config -> compiler;
            arg -> arena = arena;
            arg -> source = build_target -> sources[idx];
            arg -> build_dir = config -> build_dir;
            arg -> path_prefix = path_prefix;
            arg -> idx = idx;

            pthread_create(&threads[i], NULL, compile_object, arg);
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
            link_executable(arena, config, path_prefix, build_target, all_flags, all_flag_count, all_object_files);
            break;

        default:
            build_err("Unknown target");
    }
}

void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    make_dir(config -> build_dir);

    for (uint8_t current_target = 0; current_target < config -> target_count; current_target++) {
        Target build_target = config -> targets[current_target];

        if (build_target.type != Executable) continue;

        make_dir(build_target.output_dir);

        char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
        path_prefix[0] = '\0';

        for (uint8_t i = 0; i < config -> nest_count; i++) {
            strcat(path_prefix, "../");
        }

        size_t default_flags_size = sizeof(char*) * config -> default_flag_count;
        size_t target_flags_size = sizeof(char*) * build_target.flag_count;
        size_t all_flag_count = build_target.flag_count + config -> default_flag_count;

        char** all_flags = arena_alloc(arena, default_flags_size + target_flags_size);
        arena_memcpy(all_flags, config -> default_flags, default_flags_size);
        arena_memcpy(all_flags + config -> default_flag_count, build_target.flags, target_flags_size);

#ifdef DEBUG_MODE
        printf("DEBUG: Default flags (%d:%zuB)\n", config -> default_flag_count, default_flags_size);
        for (uint8_t i = 0; i < config -> default_flag_count; i++) {
            printf("%s\n", config -> default_flags[i]);
        }

        printf("\nDEBUG: Target flags (%d:%zuB)\n", build_target.flag_count, target_flags_size);
        for (uint8_t i = 0; i < build_target.flag_count; i++) {
            printf("%s\n", build_target.flags[i]);
        }

        printf("\n");
#endif /* ifdef DEBUG_MODE */

        char** all_object_files = arena_array(arena, char*, build_target.source_count);

        int idx = 0;
        pthread_t threads[MAX_THREADS];

        while (idx < build_target.source_count) {
            int batch_size = (build_target.source_count - idx > MAX_THREADS) ? MAX_THREADS : (build_target.source_count - idx);

            for (int i = 0; i < batch_size; i++) {
                Arg* arg = arena_alloc(arena, sizeof(*arg));

                arg -> all_object_files = &all_object_files;
                arg -> all_flags = all_flags;
                arg -> flag_count = all_flag_count;
                arg -> compiler = config -> compiler;
                arg -> arena = arena;
                arg -> source = build_target.sources[idx];
                arg -> build_dir = config -> build_dir;
                arg -> path_prefix = path_prefix;
                arg -> idx = idx;

                pthread_create(&threads[i], NULL, compile_object, arg);
                idx++;
            }

            for (int i = 0; i < batch_size; i++) {
                pthread_join(threads[i], NULL);
            }
        }

        switch (build_target.type) {
            case Executable:
            case Debug:
            case Test:
                link_executable(arena, config, path_prefix, &build_target, all_flags, all_flag_count, all_object_files);
                break;

            default:
                build_err("Unknown target");
        }
    }
}
