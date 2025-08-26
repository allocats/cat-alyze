#ifndef DEBUG_H
#define DEBUG_H

#include "../utils/result.h"
#include "../utils/arena.h"
#include "../config/config.h"

Result debug_all(Arena* arena, CatalyzeConfig* config); 
Result debug_target(Arena* arena, CatalyzeConfig* config, const char* name); 

#endif // !DEBUG_H
