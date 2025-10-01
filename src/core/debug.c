#include "debug.h"

#include "build.h"
#include "../config/config.h"
#include "../utils/arena.h"
#include "../utils/macros.h"

#define WHISKER_NOPREFIX
#include "../whisker/whisker_cmd.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void debug_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
    exit(1);
}

static void run_debug_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* name, const char* dir, const char* file) {
    build_project_target(arena, config, name);

    Whisker_Cmd cmd = {0};

    char path_prefix[(3 * config -> nest_count) + 1];
    path_prefix[0] = '\0';

    for (uint8_t i = 0; i < config -> nest_count; i++) {
        strcat(path_prefix, "../");
    }

    const size_t size = 16 + strlen(path_prefix) + strlen(dir) + strlen(file);
    char temp[size];

    snprintf(temp, size, "./%s/%s/%s", path_prefix, dir, name);
    cmd_append(&cmd, temp);
    
    if (!cmd_execute(&cmd)) {
        cmd_destroy(&cmd);
        debug_err("Failed to run");
    }

    cmd_destroy(&cmd);
}

void debug_all(ArenaAllocator* arena, CatalyzeConfig* config) {
    uint8_t count = 0;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        Target* target  = config -> targets[i];
        if (target -> type == Debug) {
            run_debug_target(arena, config, target -> name, target -> output_dir, target -> output_name);
            count++;
        }
    }

    if (UNLIKELY(count == 0)) {
        debug_err("No debug targets found");
    } 
}

void debug_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* name) {
    for (uint8_t i = 0; i < config -> target_count; i++) {
        Target* target  = config -> targets[i];
        if (target -> type != Debug && strcmp(target -> name, name) == 0) {
            debug_err("Target is not of debug type");
        } else if (target -> type != Debug && strcmp(target -> name, name) == 0) {
            run_debug_target(arena, config, target -> name, target -> output_dir, target -> output_name);
        }
    }

    debug_err("Target not found");
}
