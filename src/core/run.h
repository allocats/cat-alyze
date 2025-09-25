#ifndef RUN_H
#define RUN_H

#include "../config/config.h"

void run_project_all(ArenaAllocator* arena, CatalyzeConfig* config);
void run_project_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* target_name);

#endif // !RUN_H
