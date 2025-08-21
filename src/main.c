#include <stdio.h>

#include "utils/arena.h"
#include "utils/result.h"

#include "config/config.h"

static Arena arena = {0};

int main(int argc, char *argv[]) {
    CatalyzeConfig* config; // allocated in parse_config()
    Result result = parse_config(&arena);

    // print err function with nice formatting, check result on result

    arena_free(&arena);
}
