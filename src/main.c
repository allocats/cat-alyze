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
#include "utils/timer.h"

static ArenaAllocator arena = {0};

static void print_err(const char* msg) {
    fprintf(stderr, "\e[1mError:\e[0m %s\n", msg);
    exit(1);
}

typedef struct {
    const char* name;
    int (*handler)(int argc, char* argv[]);
    uint8_t min_args;
    uint8_t max_args;
} Command;

static int handle_build(int argc, char* argv[]);
static int handle_debug(int argc, char* argv[]);
static int handle_init(int argc, char* argv[]);
static int handle_new(int argc, char* argv[]);
static int handle_run(int argc, char* argv[]);
static int handle_test(int argc, char* argv[]);

static const Command commands[] = {
    {"build", handle_build, 2, 18},
    {"debug", handle_debug, 2, 18}, 
    {"init",  handle_init,  2, 2 },
    {"new",   handle_new,   3, 3 },
    {"run",   handle_run,   2, 3 },
    {"test",  handle_test,  2, 3 },
    {NULL,    NULL,         0, 0 } 
};

static int handle_build(int argc, char* argv[]) {
    CatalyzeConfig* config = parse_config(&arena);
    Timer timer;
    timer_start(&timer);

    if (argc == 2) {
        build_project_all(&arena, config);
    } else {
        for (uint8_t i = 2; i < argc; i++) {
            build_project_target(&arena, config, argv[i]);
        }
    }

    timer_end(&timer);
    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));

    return 0;
}

static int handle_debug(int argc, char* argv[]) {
    CatalyzeConfig* config = parse_config(&arena);
    Timer timer;
    timer_start(&timer);

    if (argc == 2) {
        debug_all(&arena, config);
    } else {
        for (uint8_t i = 2; i < argc; i++) {
            debug_target(&arena, config, argv[i]);
        }
    }

    timer_end(&timer);
    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));
    return 0;
}

static int handle_init(int argc, char* argv[]) {
    init_project();
    printf("\nCreated! Good luck with your project!\n");
    return 0;
}

static int handle_new(int argc, char* argv[]) {
    new_project(argv[2]);

    printf("\nCreated \e[1m%s\e[0m! Good luck coding and have fun :3!\n", argv[2]);
    return 0;
}

static int handle_run(int argc, char* argv[]) {
    CatalyzeConfig* config = parse_config(&arena);
    if (argc == 2) {
        run_project_all(&arena, config);
    } else {
        run_project_target(&arena, config, argv[2]);
    }

    return 0;
}

static int handle_test(int argc, char* argv[]) {
    // TODO: Testing lol
    
    CatalyzeConfig* config = parse_config(&arena);
    printf("%p : %zu bytes\n", config, sizeof(*config));
    // print_config(config);

    // if (argc == 2) {
    //     run_project_all(&arena, config);
    // } else {
    //     run_project_target(&arena, config, argv[2]);
    // }

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

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 18) {
        print_help();
        exit(1);
    }

    const Command* cmd = find_command(argv[1]);
    if (cmd == NULL) {
        print_help();
        exit(1);
    }

    if (argc < cmd -> min_args || argc > cmd -> max_args) {
        print_err("Invalid number of arguments");
        exit(1);
    }

    init_arena(&arena, 4096 / sizeof(uintptr_t));
    void* warmup = arena_alloc(&arena, 0);

    int result = cmd -> handler(argc, argv);
    exit(0);
}
