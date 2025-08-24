#include <stdio.h>

#include "core/build.h"
#include "utils/arena.h"
#include "utils/result.h"
#include "utils/timer.h"

#include "config/config.h"

static Arena arena = {0};

int main(int argc, char *argv[]) {
    Timer timer;
    CatalyzeConfig* config; // allocated in lexer_parse() called and returned by parse_config()
    Result result = parse_config(&arena);

    if (IS_ERR(result)) {
        printf("Error: %s\n", ERR_MSG(result));
        arena_free(&arena);
        return 1;
    }

    config = (CatalyzeConfig*) result.data; 

    timer_start(&timer);
    result = build_project(&arena, config, "result");
    timer_end(&timer);

    if (IS_ERR(result)) {
        printf("Error: %s\n", ERR_MSG(result));
        arena_free(&arena);
        return 1;
    } 

    printf("\nCompiling \e[1mfinished\e[0m! Took %.3f seconds\n", timer_elapsed_seconds(&timer));

    arena_free(&arena);
}
