#include "build.h"

// #define DEBUG_MODE

#define WHISKER_NOPREFIX
#include "../../whisker/cmd/whisker_cmd.h"
#include "../utils/macros.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <spawn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_THREADS 12

extern char** environ;

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

void link_executable(const char* compiler, const char* output_path, uint8_t source_count, char** all_flags, uint8_t flag_count, char** all_object_files) {
    Whisker_Cmd cmd = {0};

    cmd_append(&cmd, compiler);

    for (uint8_t i = 0; i < source_count; i++) {
        cmd_append(&cmd, all_object_files[i]);
    }

    cmd_append(&cmd, "-o");

    cmd_append(&cmd, output_path);

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

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target) {
    make_dir(config -> build_dir);

    Target build_target;
    bool found = false;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(target, config -> targets[i].name) == 0) {
            build_target = config -> targets[i];
            found = true;
            break;
        }
    }

    if (UNLIKELY(found == false)) {
        build_err("Target not found");
    }

    make_dir(config -> build_dir);

    const char* compiler = config -> compiler;
    const char* path_prefix = config -> prefix;
    const char* build_dir = config -> build_dir;
    const size_t prefix_len = config -> prefix_len;
    const size_t build_dir_len = strlen(build_dir);
    const char** default_flags = (const char**) config -> default_flags;
    const uint8_t default_flag_count = config -> default_flag_count;

    const char* output_dir = build_target.output_dir;
    const size_t output_dir_len = strlen(output_dir);

    const char* output_name = build_target.output_name;
    const size_t output_name_len = strlen(output_name);

    make_dir(output_dir);

    char* output_path = arena_alloc(arena, prefix_len + output_dir_len + output_name_len + 1);
    char* p = output_path;

    memcpy(p, path_prefix, prefix_len);
    p += prefix_len;

    memcpy(p, output_dir, output_dir_len);
    p += output_dir_len;

    *p++ = '/';

    memcpy(p, output_name, output_name_len);
    p += output_name_len;

    *p = 0;

    const uint8_t source_count = build_target.source_count;
    char** original_sources = build_target.sources;
    char** sources = arena_alloc(arena, source_count * sizeof(char*));

    for (int i = 0; i < source_count; i++ ){
        const char* source = original_sources[i];
        const size_t source_len = strlen(source);
        const size_t total_len = prefix_len + source_len + 1;

        char* source_path = arena_alloc(arena, total_len);
        char* p = source_path;

        memcpy(p, path_prefix, prefix_len);
        p += prefix_len;

        memcpy(p, source, source_len);
        p += source_len;
        *p = 0;

        sources[i] = source_path;
    }
    
    const uint8_t flag_count = build_target.flag_count;
    const uint8_t all_flag_count = default_flag_count + flag_count;

    char** all_flags = arena_alloc(arena, all_flag_count * sizeof(char*));
    memcpy(all_flags, default_flags, sizeof(char*) * default_flag_count);
    memcpy(all_flags + default_flag_count, build_target.flags, sizeof(char*) * flag_count);

    char** all_object_files = arena_array(arena, char*, source_count);

    for (int i = 0; i < source_count; i++) {
        const char* temp = source_to_object_name(arena, sources[i]);
        const size_t temp_len = strlen(temp);
        const size_t total_len = build_dir_len + prefix_len + temp_len + 1;

        char* obj_path = arena_alloc(arena, total_len);
        char* p = obj_path;

        memcpy(p, path_prefix, prefix_len);
        p += prefix_len;

        memcpy(p, build_dir, build_dir_len);
        p += build_dir_len;

        memcpy(p, temp, temp_len);
        p[temp_len] = 0;

        all_object_files[i] = obj_path;
    }

    char* argv[5 + all_flag_count];
    argv[0] = (char*) compiler;
    argv[1] = "-c";
    argv[2] = NULL;
    argv[3] = "-o";
    argv[4] = NULL;

    for (size_t j = 0; j < all_flag_count; j++) {
        argv[j + 5] = all_flags[j];
    } 

    argv[5 + all_flag_count] = NULL;

    int idx = 0;
    pid_t pids[MAX_THREADS];

    while (LIKELY(idx < source_count)) {
        int batch_size = (source_count - idx > MAX_THREADS) ? MAX_THREADS : (source_count - idx);

        for (int i = 0; i < batch_size; i++) {
            argv[2] = (char*) sources[idx];
            argv[4] = all_object_files[idx];
            posix_spawnp(&pids[i], argv[0], NULL, NULL, argv, environ);
            idx++;
        }

        for (int i = 0; i < batch_size; i++) {
            int status;
            if (UNLIKELY(waitpid(pids[i], &status, 0) < 0)) {
                build_err("Build failure");
            }

            if (UNLIKELY(!WIFEXITED(status)) || WEXITSTATUS(status) != 0) {
                build_err("Compilation failed");
            }
        }

        switch (build_target.type) {
            case Executable:
            case Debug:
            case Test:
                link_executable(compiler, output_path, source_count, all_flags, all_flag_count, all_object_files);
                break;

            default:
                build_err("Unknown target");
        }
    }
}

void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    make_dir(config -> build_dir);

    const char* compiler = config -> compiler;
    const char* path_prefix = config -> prefix;
    const char* build_dir = config -> build_dir;
    const size_t prefix_len = config -> prefix_len;
    const size_t build_dir_len = strlen(build_dir);
    const char** default_flags = (const char**) config -> default_flags;
    const uint8_t default_flag_count = config -> default_flag_count;

    for (uint8_t current_target = 0; current_target < config -> target_count; current_target++) {
        Target build_target = config -> targets[current_target];
        if (build_target.type != Executable) continue;

        const char* output_dir = build_target.output_dir;
        const size_t output_dir_len = strlen(output_dir);

        const char* output_name = build_target.output_name;
        const size_t output_name_len = strlen(output_name);

        make_dir(output_dir);

        char* output_path = arena_alloc(arena, prefix_len + output_dir_len + output_name_len + 2);
        char* p = output_path;

        memcpy(p, path_prefix, prefix_len);
        p += prefix_len;

        memcpy(p, output_dir, output_dir_len);
        p += output_dir_len;

        *p++ = '/';

        memcpy(p, output_name, output_name_len);
        p += output_name_len;

        *p = 0;

        const uint8_t source_count = build_target.source_count;
        char** original_sources = build_target.sources;
        char** sources = arena_alloc(arena, source_count * sizeof(char*));

        for (int i = 0; i < source_count; i++ ){
            const char* source = original_sources[i];
            const size_t source_len = strlen(source);
            const size_t total_len = prefix_len + source_len + 1;

            char* source_path = arena_alloc(arena, total_len);
            char* p = source_path;

            memcpy(p, path_prefix, prefix_len);
            p += prefix_len;

            memcpy(p, source, source_len);
            p += source_len;
            *p = 0;

            sources[i] = source_path;
        }

        const uint8_t flag_count = build_target.flag_count;
        const uint8_t all_flag_count = default_flag_count + flag_count;

        char** all_flags = arena_alloc(arena, all_flag_count * sizeof(char*));
        memcpy(all_flags, default_flags, sizeof(char*) * default_flag_count);
        memcpy(all_flags + default_flag_count, build_target.flags, sizeof(char*) * flag_count);

        char** all_object_files = arena_array(arena, char*, build_target.source_count);

        for (int i = 0; i < source_count; i++) {
            const char* temp = source_to_object_name(arena, sources[i]);
            const size_t temp_len = strlen(temp);
            const size_t total_len = build_dir_len + prefix_len + temp_len + 1;

            char* obj_path = arena_alloc(arena, total_len);
            char* p = obj_path;

            memcpy(p, path_prefix, prefix_len);
            p += prefix_len;
            
            memcpy(p, build_dir, build_dir_len);
            p += build_dir_len;

            memcpy(p, temp, temp_len);
            p += temp_len;
            *p = 0;

            all_object_files[i] = obj_path;
        }

        char* argv[5 + all_flag_count];
        argv[0] = (char*) compiler;
        argv[1] = "-c";
        argv[2] = NULL;
        argv[3] = "-o";
        argv[4] = NULL;

        for (size_t j = 0; j < all_flag_count; j++) {
            argv[j + 5] = all_flags[j];
        } 
        
        argv[5 + all_flag_count] = NULL;

        int idx = 0;
        pid_t pids[MAX_THREADS];

        while (LIKELY(idx < source_count)) {
            int batch_size = (source_count - idx > MAX_THREADS) ? MAX_THREADS : (source_count - idx);

            for (int i = 0; i < batch_size; i++) {
                argv[2] = (char*) sources[idx];
                argv[4] = all_object_files[idx];
                posix_spawnp(&pids[i], argv[0], NULL, NULL, argv, environ);
                idx++;
            }

            for (int i = 0; i < batch_size; i++) {
                int status;
                if (UNLIKELY(waitpid(pids[i], &status, 0) < 0)) {
                    build_err("Build failure");
                }

                if (UNLIKELY(!WIFEXITED(status)) || WEXITSTATUS(status) != 0) {
                    build_err("Compilation failed");
                }
            }
        }

        switch (build_target.type) {
            case Executable:
            case Debug:
            case Test:
                link_executable(compiler, output_path, source_count, all_flags, all_flag_count, all_object_files);
                break;

            default:
                build_err("Unknown target");
        }
    }
}
