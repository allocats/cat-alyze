#include "run.h"

#include "build.h"

#include "../utils/arena.h"

#define WHISKER_NOPREFIX
#include "../../whisker/cmd/whisker_cmd.h"

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
        Target target = config -> targets[i];

        if (target.type != Executable) continue;

        Whisker_Cmd cmd = {0};

        const char* path_prefix = config -> prefix;
        const size_t size = 16 + config -> prefix_len + strlen(target.output_dir) + strlen(target.output_name);

        char temp[size];
        snprintf(temp, size, "./%s%s/%s", path_prefix, target.output_dir, target.output_name);

        cmd_append(&cmd, temp); 

        if (!cmd_execute(&cmd)) {
            cmd_destroy(&cmd);
            run_err("Run failed");
        }

        cmd_destroy(&cmd);
    }
}

void run_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target_name) {
    Target* target = NULL;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(config -> targets[i].name, target_name) == 0) {
            target = &config -> targets[i];
            break;
        }
    }

    if (target == NULL) {
        run_err("Target not found");
    }

    build_project_target(arena, config, target_name);

    Whisker_Cmd cmd = {0};

    const char* path_prefix = config -> prefix;
    const size_t size = 16 + config -> prefix_len + strlen(target -> output_dir) + strlen(target -> output_name);

    char temp[size];
    snprintf(temp, size, "./%s%s/%s", path_prefix, target -> output_dir, target -> output_name);

    cmd_append(&cmd, temp); 

    if (!cmd_execute(&cmd)) {
        cmd_destroy(&cmd);
        run_err("Run failed");
    }

    cmd_destroy(&cmd);
}
