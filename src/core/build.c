#include "build.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

Result make_build_dir(const char* dir) {
    size_t mkdir_size = 32 + MAX_BUILD_DIR_LEN;
    char mkdir_cmd[mkdir_size];

    size_t mkdir_offset = snprintf(mkdir_cmd, mkdir_size, "%s", "mkdir -p ");
    mkdir_offset += snprintf(mkdir_cmd + mkdir_offset, mkdir_size - mkdir_offset, "%s", dir);

    if (system(mkdir_cmd) != 0) {
        return err("Mkdir failed");
    }

    return ok(NULL);
}

Result make_output_dir(const char* dir) {
    size_t mkdir_size = 32 + MAX_BUILD_DIR_LEN;
    char mkdir_cmd[mkdir_size];

    size_t mkdir_offset = snprintf(mkdir_cmd, mkdir_size, "%s", "mkdir -p ");
    mkdir_offset += snprintf(mkdir_cmd + mkdir_offset, mkdir_size - mkdir_offset, "%s", dir);

    if (system(mkdir_cmd) != 0) {
        return err("Mkdir failed");
    }

    return ok(NULL);
}

char* source_to_object_name(ArenaAllocator* arena, const char* source_path) {
    const char* name = strrchr(source_path, '/');
    name = name ? name + 1 : source_path;

    const char* ext = strrchr(name, '.');
    size_t len = ext ? (size_t)(ext - name) : strlen(name);

    char* object_name = arena_alloc(arena, len + 3);
    strncpy(object_name, name, len);
    strcpy(object_name + len, ".o");

    return object_name;
}

Result link_executable(CatalyzeConfig* config, const char* path_prefix, Target* build_target, char** all_flags, uint8_t flag_count, char** all_object_files) {
    size_t size = 32 + strlen(config -> compiler) + 1; 
    size += strlen(path_prefix) + 1;
    size += strlen(build_target -> output_dir);
    size += strlen(build_target -> output_name);
    
    for (uint8_t i = 0; i < flag_count; i++) {
        size += strlen(all_flags[i]) + 1;
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        size += strlen(all_object_files[i]) + 1;
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
    offset += snprintf(cmd + offset, size - offset, " %s%s%s", path_prefix, build_target -> output_dir, build_target -> output_name);

    if (system(cmd) != 0) {
        return err("compiler failed");
    }
 
    return ok(NULL);
}

Result build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target) {
    Result result = make_build_dir(config -> build_dir);

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    Target* build_target = NULL;
    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(target, config -> targets[i] -> name) == 0) {
            build_target = config -> targets[i];
            break;
        }
    }

    if (build_target == NULL) {
        return err("Target not found");
    }

    result = make_output_dir(build_target -> output_dir);
    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    char** all_flags = arena_array(arena, char*, (config -> default_flag_count + build_target -> flag_count));
    uint8_t flag_count = 0;

    for (uint8_t i = 0; i < config -> default_flag_count; i++) {
        all_flags[flag_count++] = config -> default_flags[i];
    }

    for (uint8_t i = 0; i < build_target -> flag_count; i++) {
        all_flags[flag_count++] = build_target -> flags[i];
    }

    char** all_object_files = arena_array(arena, char*, build_target -> source_count);

    for (uint8_t k = 0; k < build_target -> source_count; k++) {
        size_t size = 32 + strlen(config -> compiler) + 1;
        for (int i = 0; i < flag_count; i++) {
            size += strlen(all_flags[i]) + 1;
        }

        char* object_file = source_to_object_name(arena, build_target -> sources[k]);

        size += strlen(build_target -> sources[k]) + 1;
        size += strlen(path_prefix) + 1;
        size += strlen(config -> build_dir) + 1;
        size += strlen(object_file) + 1;
        size += strlen(path_prefix) + 1;
 
        size = align_size(size);
        char cmd[size];

        size_t offset = snprintf(cmd, size, "%s", config -> compiler);

        for (uint8_t i = 0; i < flag_count; i++) {
            offset += snprintf(cmd + offset, size - offset, " %s", all_flags[i]); 
        }

        offset += snprintf(cmd + offset, size - offset, " -c");
        offset += snprintf(cmd + offset, size - offset, " %s%s", path_prefix, build_target -> sources[k]);

        offset += snprintf(cmd + offset, size - offset, " -o");
        offset += snprintf(cmd + offset, size - offset, " %s%s%s", path_prefix, config -> build_dir, object_file);

        all_object_files[k] = arena_strdup(arena, object_file);

        if (system(cmd) != 0) {
            return err("Failed to compile");
        }
    }

    switch (build_target -> type) {
        case Executable:
            result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
            break;

        case Debug:
            result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
            break;

        case Test:
            result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
            break;

        default:
            return err("Unknown target");
    }

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    return ok(NULL);
}

Result build_project_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    Result result = make_build_dir(config -> build_dir);

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    for (uint8_t current_target = 0; current_target < config -> target_count; current_target++) {
        Target* build_target = config -> targets[current_target];

        if (build_target == NULL) {
            return err("Target not found");
        }

        if (build_target -> type == Debug || build_target -> type == Test) continue;

        result = make_output_dir(build_target -> output_dir);
        if (IS_ERR(result)) {
            return err(ERR_MSG(result));
        }

        char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
        path_prefix[0] = '\0';

        for (uint8_t i = 0; i < config -> nest_count; i++) {
            strcat(path_prefix, "../");
        }

        char** all_flags = arena_array(arena, char*, (config -> default_flag_count + build_target -> flag_count));
        uint8_t flag_count = 0;

        for (uint8_t i = 0; i < config -> default_flag_count; i++) {
            all_flags[flag_count++] = config -> default_flags[i];
        }

        for (uint8_t i = 0; i < build_target -> flag_count; i++) {
            all_flags[flag_count++] = build_target -> flags[i];
        }

        char** all_object_files = arena_array(arena, char*, build_target -> source_count);

        for (uint8_t k = 0; k < build_target -> source_count; k++) {
            size_t size = 32 + strlen(config -> compiler) + 1;
            for (int i = 0; i < flag_count; i++) {
                size += strlen(all_flags[i]) + 1;
            }

            char* object_file = source_to_object_name(arena, build_target -> sources[k]);

            size += strlen(build_target -> sources[k]) + 1;
            size += strlen(path_prefix) + 1;
            size += strlen(config -> build_dir) + 1;
            size += strlen(object_file) + 1;
            size += strlen(path_prefix) + 1;

            size = align_size(size);
            char cmd[size];

            size_t offset = snprintf(cmd, size, "%s", config -> compiler);

            for (uint8_t i = 0; i < flag_count; i++) {
                offset += snprintf(cmd + offset, size - offset, " %s", all_flags[i]); 
            }

            offset += snprintf(cmd + offset, size - offset, " -c");
            offset += snprintf(cmd + offset, size - offset, " %s%s", path_prefix, build_target -> sources[k]);

            offset += snprintf(cmd + offset, size - offset, " -o");
            offset += snprintf(cmd + offset, size - offset, " %s%s%s", path_prefix, config -> build_dir, object_file);

            all_object_files[k] = arena_strdup(arena, object_file);

            if (system(cmd) != 0) {
                return err("Failed to compile");
            }
        }

        switch (build_target -> type) {
            case Executable:
                result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
                break;

            case Debug:
                result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
                break;

            case Test:
                result = link_executable(config, path_prefix, build_target, all_flags, flag_count, all_object_files);
                break;

            default:
                return err("Unknown target");
        }

        if (IS_ERR(result)) {
            return err(ERR_MSG(result));
        }
    }

    return ok(NULL);
}
