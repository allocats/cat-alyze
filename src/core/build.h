#ifndef BUILD_H
#define BUILD_H

#include "../config/config.h"

#include "../utils/arena.h"
#include "../utils/result.h"

Result build_project(Arena* arena, CatalyzeConfig* config, const char* target);

#endif // !BUILD_H
