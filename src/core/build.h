#ifndef BUILD_H
#define BUILD_H

#include "../config/config.h"

#include "../utils/arena.h"

#include <stdint.h>

typedef struct {
    ArenaAllocator* arena;
    char** all_flags;
    uint8_t flag_count;
    char* source;
    const char* path_prefix;
    size_t prefix_len;
    char* build_dir;
    const char* object_file;
    char* compiler;
} Arg;

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target);
void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config);

void make_dir(const char* dir); 

#endif // !BUILD_H
