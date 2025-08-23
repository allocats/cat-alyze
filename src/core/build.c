#include "build.h"

#include <stdint.h>
#include <stdio.h>

Result build_project(Arena* arena, CatalyzeConfig* config, const char* target) {
    char cmd[2048];

    sprintf(cmd, "%s", config -> compiler);

    for (uint8_t i = 0; i < config -> default_flag_count; i++) {
        sprintf(cmd, "%s %s", cmd, config -> default_flags[i]);
    }

    sprintf(cmd, "%s -c", cmd);

    Target* build_target = NULL;

    for (uint8_t i = 0; i < config -> target_count; i++) {
        if (strcmp(config -> targets[i].name, target) == 0) {
            build_target = &config -> targets[i];
            break;
        }
    }

    if (build_target == NULL) {
        return err("Target not found");
    }

    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        sprintf(cmd, "%s %s", cmd, build_target -> sources[i]);
    }

    sprintf(cmd, "%s -o", cmd);
    for (uint8_t i = 0; i < build_target -> source_count; i++) {
        sprintf(cmd, "%s %s%s.o", cmd, config -> build_dir, build_target -> sources[i]);
    }

    printf("%s\n", cmd);

    return ok(NULL);
}
