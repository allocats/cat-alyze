#include <stdio.h>

#include "utils/arena.h"
#include "utils/result.h"

#include "config/config.h"

static Arena arena = {0};

int main(int argc, char *argv[]) {
    CatalyzeConfig* config; // allocated in parse_config()
    Result result = parse_config(&arena);

    if (IS_ERR(result)) {
        printf("Main: %s\n", ERR_MSG(result));
        arena_free(&arena);
        return 1;
    }

    config = (CatalyzeConfig*) result.data; 

    printf("Parsed %d flags:\n", config->flag_count);
    for(int i = 0; i < config->flag_count; i++) {
        printf("  [%d]: '%s'\n", i, config->flags[i]);
    }

    arena_free(&arena);
}
