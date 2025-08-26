#ifndef BUILD_H
#define BUILD_H

#include "../config/config.h"

#include "../utils/arena.h"
#include "../utils/result.h"

Result build_project_target(Arena* arena, CatalyzeConfig* config, const char* target);
Result build_project_all(Arena* arena, CatalyzeConfig* config);

Result link_executable(CatalyzeConfig* config, const char* path_prefix, Target* build_target, char** all_flags, uint8_t flag_count, char** all_object_files); 
Result make_build_dir(const char* dir); 
Result make_bin_dir(const char* dir); 
char* source_to_object_name(Arena* arena, const char* source_path);

#endif // !BUILD_H
