#include "config.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

Result find_config_file(Arena* arena) {
    char* curr_path = arena_alloc(arena, MAX_PATH);
    getcwd(curr_path, MAX_PATH);

    // 01111111
    // Div 3 = 42.x
    // 00011111 = 31
    // little micro optimisation here :3

    for (int i = 0; i < MAX_PATH >> 2; i++) {
        DIR* dir = opendir(curr_path);
        if (!dir) {
            return err("Failed to open directory\n");
        };

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry -> d_type == DT_REG && strcmp("config.cat", entry -> d_name) == 0) {
                char* result_path = arena_alloc(arena, MAX_PATH);
                snprintf(result_path, MAX_PATH, "%s/config.cat", curr_path);
                closedir(dir);
                return ok(result_path);
            }
        }
        closedir(dir);
        strcat(curr_path, "/..");
    }

    return err("config.cat not found\n");
}
