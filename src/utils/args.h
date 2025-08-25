#ifndef ARGS_H
#define ARGS_H

#include "../config/config.h"

#include <cstdint>
#include <string.h>

/*
*
*  init - create in current directory
*  new [name] - creates new dir and creates project in it 
*  bulid <target> - Optional, by default builds all? 
*  run <target> - Optional, if more than one targets then specify 
*
*  thinkking aobut moving this into main but not sure, i sleep now then decide tmrw
*
*/

static void parse_args(uint8_t count, char** args, CatalyzeConfig* config) {
    if (count == 2 && args[1][0] == 'i') {
        if (strncmp(args[1], "init", 4) == 0) {
            // init_project()
        }
    }
}


#endif // !define ARGS_H
