#include "debug.h"

#include "../utils/result.h"
#include "../utils/arena.h"
#include "../config/config.h"
#include "build.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

static Result build_debug_target(ArenaAllocator* arena, CatalyzeConfig* config, Target* target) {
    Result result = make_build_dir(config -> build_dir);

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    result = make_output_dir(target -> output_dir);
    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    char* path_prefix = arena_alloc(arena, (config -> nest_count * 3) + 1);
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    char** all_flags = arena_array(arena, char*, (config -> default_flag_count + target -> flag_count));
    uint8_t flag_count = config -> default_flag_count + target -> flag_count;

    if (config -> default_flag_count > 0) {
        arena_memcpy(all_flags, config -> default_flags, config -> default_flag_count * sizeof(char*));
    }

    if (target -> flag_count > 0) {
        arena_memcpy(all_flags + config -> default_flag_count, target -> flags, target-> flag_count * sizeof(char*));
    }

    char** all_object_files = arena_array(arena, char*, target -> source_count);

    for (uint8_t i = 0; i < target -> source_count; i++) {
        size_t size = 32 + strlen(config -> compiler) + 1;
        for (int i = 0; i < flag_count; i++) {
            size += strlen(all_flags[i]) + 1;
        }

        char* object_file = source_to_object_name(arena, target -> sources[i]);

        size += strlen(target -> sources[i]) + 1;
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
        offset += snprintf(cmd + offset, size - offset, " %s%s", path_prefix, target -> sources[i]);

        offset += snprintf(cmd + offset, size - offset, " -o");
        offset += snprintf(cmd + offset, size - offset, " %s%s%s", path_prefix, config -> build_dir, object_file);

        all_object_files[i] = arena_strdup(arena, object_file);

        if (system(cmd) != 0) {
            return err("Failed to compile");
        }
    }

    result = link_executable(config, path_prefix, target, all_flags, flag_count, all_object_files);

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    return ok(NULL);
}

static Result run_debug_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* name) {
    Target* target = NULL;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (config -> targets[i] -> type == Debug && strcmp(config -> targets[i] -> name, name) == 0) {
            target = config -> targets[i];
            break;
        }
    }

    if (target == NULL) {
        return err("Target not found");
    }

    Result result = build_debug_target(arena, config, target);

    if (IS_ERR(result)) {
        return err(ERR_MSG(result));
    }

    size_t size = 32 + strlen(target -> output_dir) + strlen(target -> output_name) + (3 * config -> nest_count);
    char cmd[size];
    char path_prefix[(3 * config -> nest_count) + 1];
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    snprintf(cmd, size, "./%s%s%s", path_prefix, target -> output_dir, target -> output_name);

    if (system(cmd) != 0) {
        return err("Run failed");
    }

    return ok(NULL);
}

Result debug_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    Result result;
    uint8_t count = 0;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (config -> targets[i] -> type == Debug) {
            result = run_debug_target(arena, config, config -> targets[i] -> name);

            if (IS_ERR(result)) {
                return err(ERR_MSG(result));
            }

            count++;
        }
    }

    if (count == 0) {
        return err("No debug targets found");
    } else {
        return ok(NULL);
    }
}

Result debug_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* name) {
    Result result;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (config -> targets[i] -> type != Debug && strcmp(config -> targets[i] -> name, name) == 0) {
            return err("Target is not of debug type");
        } else if (config -> targets[i] -> type != Debug && strcmp(config -> targets[i] -> name, name) == 0) {
            result = run_debug_target(arena, config, config -> targets[i] -> name);

            if (IS_ERR(result)) {
                return err(ERR_MSG(result));
            } else {
                return ok(NULL);
            }
        }
    }

    return err("Target not found");
}
