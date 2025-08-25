#ifndef RUN_H
#define RUN_H

#include "../config/config.h"
#include "../utils/result.h"

Result run_project_all(Arena* arena, CatalyzeConfig* config);
Result run_project_target(Arena* arena, CatalyzeConfig* config, const char* target_name);

#endif // !RUN_H
