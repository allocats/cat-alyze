#include "build.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static char* source_to_object_name(Arena* arena, const char* source_path) {
    const char* name = strrchr(source_path, '/');
    name = name ? name + 1 : source_path;

    const char* ext = strrchr(name, '.');
    size_t len = ext ? (size_t)(ext - name) : strlen(name);

    char* object_name = arena_alloc(arena, len + 3);
    strncpy(object_name, name, len);
    strcpy(object_name + len, ".o");

    return object_name;
}

Result build_project(Arena* arena, CatalyzeConfig* config, const char* target) {
    Target* build_target = NULL;
    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(target, config -> targets[i].name) == 0) {
            build_target = &config -> targets[i];
            break;
        }
    }

    if (build_target == NULL) {
        return err("Target not found");
    }

    printf("\nCount: %d\n", build_target -> source_count);

    char** all_flags = arena_array(arena, char*, (config -> default_flag_count + build_target -> flag_count));
    uint8_t flag_count = 0;

    for (uint8_t i = 0; i < config -> default_flag_count; i++) {
        all_flags[flag_count++] = config -> default_flags[i];
    }

    for (uint8_t i = 0; i < build_target -> flag_count; i++) {
        all_flags[flag_count++] = build_target -> flags[i];
    }

    char** all_object_files = arena_array(arena, char*, build_target -> source_count);

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        size_t size = 32 + strlen(config -> compiler) + 1;
        for (int i = 0; i < flag_count; i++) {
            size += strlen(all_flags[i]) + 1;
        }

        char* object_file = source_to_object_name(arena, build_target -> sources[i]);

        size += strlen(build_target -> sources[i]) + 1;
        size += strlen(config -> build_dir) + 1;
        size += strlen(object_file) + 1;

        size = align_size(size);
        char cmd[size];

        size_t offset = snprintf(cmd, size, "%s", config -> compiler);

        for (uint8_t i = 0; i < flag_count; i++) {
            offset += snprintf(cmd + offset, size - offset, " %s", all_flags[i]); 
        }

        offset += snprintf(cmd + offset, size - offset, " -c");
        offset += snprintf(cmd + offset, size - offset, " %s", build_target -> sources[i]);

        offset += snprintf(cmd + offset, size - offset, " -o");
        offset += snprintf(cmd + offset, size - offset, " %s%s", config -> build_dir, object_file);

        all_object_files[i] = arena_strdup(arena, object_file);

        if (system(cmd) != 0) {
            return err("failed to compile");
        }
    }

    size_t size = 32 + strlen(config -> compiler) + 1; 
    size += strlen(build_target -> output_dir);
    size += strlen(build_target -> output_name);
    
    for (uint8_t i = 0; i < flag_count; i++) {
        size += strlen(all_flags[i]) + 1;
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        size += strlen(all_object_files[i]) + 1;
    }

    char cmd[size];

    size_t offset = snprintf(cmd, size, "%s", config -> compiler);

    for (uint8_t i = 0; i < flag_count; i++) {
        offset += snprintf(cmd + offset, size - offset, " %s", all_flags[i]);
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        offset += snprintf(cmd + offset, size - offset, " %s%s", config -> build_dir, all_object_files[i]);
    }

    offset += snprintf(cmd + offset, size - offset, " -o");
    offset += snprintf(cmd + offset, size - offset, " %s%s", build_target -> output_dir, build_target -> output_name);

    if (system(cmd) != 0) {
        return err("compiler failed");
    }
 
    return ok(NULL);
}
