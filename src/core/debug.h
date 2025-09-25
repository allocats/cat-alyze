#ifndef DEBUG_H
#define DEBUG_H

#include "../utils/arena.h"
#include "../config/config.h"

void debug_all(ArenaAllocator* arena, CatalyzeConfig* config); 
void debug_target(ArenaAllocator* arena, CatalyzeConfig* config, const char* name); 

#endif // !DEBUG_H
