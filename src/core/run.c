#include "run.h"

#include "build.h"

#include "../utils/arena.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void run_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
    exit(1);
}

void run_project_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    build_project_all(arena, config);

    for (uint8_t i = 0; i < config -> target_count; i++) {
        Target* target = config -> targets[i];

        if (target == NULL) {
            run_err("Invalid target found");
        }

        if (target -> type != Executable) continue;

        size_t size = 32 + strlen(target -> output_dir) + strlen(target -> output_name) + (3 * config -> nest_count);
        char cmd[size];
        char path_prefix[(3 * config -> nest_count) + 1];
        path_prefix[0] = '\0';
        
        for (uint8_t i = 0; i < config -> nest_count; i++) {
            strcat(path_prefix, "../");
        }

        snprintf(cmd, size, "./%s%s/%s", path_prefix, target -> output_dir, target -> output_name);

        if (system(cmd) != 0) {
            run_err("Run failed");
        }
    }
}

void run_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target_name) {
    Target* target = NULL;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(config -> targets[i] -> name, target_name) == 0) {
            target = config -> targets[i];
            break;
        }
    }

    if (target == NULL) {
        run_err("Target not found");
    }

    build_project_target(arena, config, target_name);

    size_t size = 32 + strlen(target -> output_dir) + strlen(target -> output_name) + (3 * config -> nest_count);
    char cmd[size];
    char path_prefix[(3 * config -> nest_count) + 1];
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    snprintf(cmd, size, "./%s%s/%s", path_prefix, target -> output_dir, target -> output_name);

    if (system(cmd) != 0) {
        run_err("Run failed");
    }
}
