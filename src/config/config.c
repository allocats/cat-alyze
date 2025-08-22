#include "config.h"
#include "lexer.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

Result find_config_file(Arena* arena) {
    char* curr_path = arena_alloc(arena, MAX_PATH);
    getcwd(curr_path, MAX_PATH);

    // little micro optimisation here, saved some nanoseconds :3 
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

    return err("config.cat not found");
}

Result parse_config(Arena* arena) { 
    Result path = find_config_file(arena);

    if (IS_ERR(path)) {
        return err(ERR_MSG(path));
    } 

    FILE* fptr = fopen(path.data, "r");

    if (!fptr) return err("failed to open config.cat");
    
    fseek(fptr, 0, SEEK_END);
    uint32_t len = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    char* buffer = arena_alloc(arena, len + 1);
    fread(buffer, 1, len, fptr); 
    buffer[len] = '\0';

    fclose(fptr);

    return lexer_parse(arena, buffer);
}
