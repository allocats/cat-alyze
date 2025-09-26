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
} Cmd;

void build_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target);
void build_project_all(ArenaAllocator* arena, CatalyzeConfig* config);

void link_executable(CatalyzeConfig* config, const char* path_prefix, Target* build_target, char** all_flags, uint8_t flag_count, char** all_object_files); 
void make_dir(const char* dir); 
char* source_to_object_name(ArenaAllocator* arena, const char* source_path);

#endif // !BUILD_H
