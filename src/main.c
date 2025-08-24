#include <stdio.h>

#include "core/build.h"
#include "utils/arena.h"
#include "utils/result.h"

#include "config/config.h"

static Arena arena = {0};

int main(int argc, char *argv[]) {
    CatalyzeConfig* config; // allocated in lexer_parse() called and returned by parse_config()
    Result result = parse_config(&arena);

    if (IS_ERR(result)) {
        printf("Main: %s\n", ERR_MSG(result));
        arena_free(&arena);
        return 1;
    }

    config = (CatalyzeConfig*) result.data; 

    print_config(config);
    result = build_project(&arena, config, "result");

    if (IS_ERR(result)) {
        printf("Error: %s\n", ERR_MSG(result));
        arena_free(&arena);
        return 1;
    }

    arena_free(&arena);
}
