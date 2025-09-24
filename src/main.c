#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config/config.h"

#include "core/build.h"
#include "core/debug.h"
#include "core/init.h"
#include "core/new.h"
#include "core/run.h"

#include "utils/arena.h"
#include "utils/help.h"
#include "utils/result.h"
#include "utils/timer.h"

static ArenaAllocator arena = {0};

static void print_err(const char* msg) {
    fprintf(stderr, "\e[1mError:\e[0m %s\n", msg);
}

typedef struct {
    const char* name;
    int (*handler)(int argc, char* argv[]);
    uint8_t min_args;
    uint8_t max_args;
    bool config;
} Command;

static int handle_build(int argc, char* argv[]);
static int handle_debug(int argc, char* argv[]);
static int handle_init(int argc, char* argv[]);
static int handle_new(int argc, char* argv[]);
static int handle_run(int argc, char* argv[]);
static int handle_test(int argc, char* argv[]);

static const Command commands[] = {
    {"build", handle_build, 2, 18, true},
    {"debug", handle_debug, 2, 18, true}, 
    {"init",  handle_init,  2, 2,  false},
    {"new",   handle_new,   3, 3,  false},
    {"run",   handle_run,   2, 3,  true},
    {"test",  handle_test,  2, 3,  true},
    {NULL,    NULL,         0, 0,  false} 
};

static int handle_build(int argc, char* argv[]) {
    Result result = parse_config(&arena);
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    CatalyzeConfig* config = (CatalyzeConfig*) result.data;
    Timer timer;
    timer_start(&timer);

    if (argc == 2) {
        result = build_project_all(&arena, config);

        if (IS_ERR(result)) {
            print_err(ERR_MSG(result));
            return 1;
        }
    } else {
        for (uint8_t i = 2; i < argc; i++) {
            result = build_project_target(&arena, config, argv[i]);

            if (IS_ERR(result)) {
                print_err(ERR_MSG(result));
                return 1;
            }
        }
    }

    timer_end(&timer);
    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));

    return 0;
}

static int handle_debug(int argc, char* argv[]) {
    Result result = parse_config(&arena);
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    CatalyzeConfig* config = (CatalyzeConfig*) result.data;
    Timer timer;
    timer_start(&timer);

    if (argc == 2) {
        result = debug_all(&arena, config);

        if (IS_ERR(result)) {
            print_err(ERR_MSG(result));
            return 1;
        }
    } else {
        for (uint8_t i = 2; i < argc; i++) {
            result = debug_target(&arena, config, argv[i]);

            if (IS_ERR(result)) {
                print_err(ERR_MSG(result));
                return 1;
            }
        }
    }

    timer_end(&timer);
    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));
    return 0;
}

static int handle_init(int argc, char* argv[]) {
    Result result = init_project();
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    printf("\nCreated! Good luck with your project!\n");
    return 0;
}

static int handle_new(int argc, char* argv[]) {
    Result result = new_project(argv[2]);
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    printf("\nCreated \e[1m%s\e[0m! Good luck coding and have fun :3!\n", argv[2]);
    return 0;
}

static int handle_run(int argc, char* argv[]) {
    Result result = parse_config(&arena);
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    CatalyzeConfig* config = (CatalyzeConfig*) result.data;

    if (argc == 2) {
        result = run_project_all(&arena, config);
    } else {
        result = run_project_target(&arena, config, argv[2]);
    }

    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    return 0;
}

static int handle_test(int argc, char* argv[]) {
    // TODO: Testing lol
    
    Result result = parse_config(&arena);
    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }
    
    CatalyzeConfig* config = (CatalyzeConfig*) result.data;

    if (argc == 2) {
        result = run_project_all(&arena, config);
    } else {
        result = run_project_target(&arena, config, argv[2]);
    }

    if (IS_ERR(result)) {
        print_err(ERR_MSG(result));
        return 1;
    }

    return 0;
}

static const Command* find_command(const char* name) {
    for (const Command* cmd = commands; cmd -> name != NULL; cmd++) {
        if (strcmp(cmd -> name, name) == 0) {
            return cmd;
        }
    }

    return NULL;
}

static void cleanup_and_exit(int code) {
    // arena_free(&arena);
    exit(code);
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 18) {
        print_help();
        cleanup_and_exit(1);
    }

    const Command* cmd = find_command(argv[1]);
    if (cmd == NULL) {
        print_help();
        cleanup_and_exit(1);
    }

    if (argc < cmd -> min_args || argc > cmd -> max_args) {
        print_err("Invalid number of arguments");
        cleanup_and_exit(1);
    }

    init_arena(&arena, 0);

    int result = cmd -> handler(argc, argv);
    cleanup_and_exit(0);
}
