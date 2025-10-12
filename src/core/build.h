#ifndef BUILD_H
#define BUILD_H

#include "../config/config.h"

#include "../utils/arena.h"

#include <stdint.h>

typedef struct {
    ArenaAllocator* arena;
    char* compiler;
    char** all_flags;
    uint8_t flag_count;
    char* source;
    char* path_prefix;
    char* build_dir;
    char*** all_object_files;
    uint8_t idx;
} Arg;

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target);
void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config);

void make_dir(const char* dir); 

#endif // !BUILD_H
