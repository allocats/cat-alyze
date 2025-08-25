#ifndef BUILD_H
#define BUILD_H

#include "../config/config.h"

#include "../utils/arena.h"
#include "../utils/result.h"

Result build_project_target(Arena* arena, CatalyzeConfig* config, const char* target);
Result build_project_all(Arena* arena, CatalyzeConfig* config);

#endif // !BUILD_H
