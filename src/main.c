#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "config/config.h"

#include "core/build.h"
#include "core/init.h"
#include "core/new.h"
#include "core/run.h"

#include "utils/arena.h"
#include "utils/help.h"
#include "utils/result.h"
#include "utils/timer.h"

static Arena arena = {0};

static void print_err(const char* msg) {
    fprintf(stderr, "\e[1mError:\e[0m %s\n", msg);
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 18) {
        print_help();
        arena_free(&arena);
        return 1;
    }

    CatalyzeConfig* config; // allocated in lexer_parse() called and returned by parse_config()
    Result result;

    switch (argv[1][0]) {
        case 'b':
            if (strcmp(argv[1], "build") == 0) {
                result = parse_config(&arena);

                if (IS_ERR(result)) {
                    print_err(ERR_MSG(result));
                    arena_free(&arena);
                    return 1;
                }

                config = (CatalyzeConfig*) result.data; 

                Timer timer;
                if (argc == 2) {
                    timer_start(&timer);
                    result = build_project_all(&arena, config);
                    timer_end(&timer);

                    if (IS_ERR(result)) {
                        print_err(ERR_MSG(result));
                        arena_free(&arena);
                        return 1;
                    }

                    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));
                } else {
                    uint8_t i = 2;

                    timer_start(&timer);
                    while (i < argc) {
                        result = build_project_target(&arena, config, argv[i]);

                        if (IS_ERR(result)) {
                            print_err(ERR_MSG(result));
                            arena_free(&arena);
                            return 1;
                        }

                        i++;
                    }
                    timer_end(&timer);
                    printf("\nCompiling \e[1mfinished\e[0m! Built all targets. Took %.3f seconds\n", timer_elapsed_seconds(&timer));
                }
            } else {
                print_help();
                arena_free(&arena);
                return 1;
            }
            break;

        case 'i':
            if (strcmp(argv[1], "init") == 0) {
                if (argc != 2) {
                    print_err("Invalid, expects: catalyze init");
                    arena_free(&arena);
                    return 1;
                }

                result = init_project();
                if (IS_ERR(result)) {
                    print_err(ERR_MSG(result));
                    arena_free(&arena);
                    return 1;
                }

                printf("\nCreated! Happy coding!\n");
            } else {
                print_help();
                arena_free(&arena);
                return 1;
            }
            break;

        case 'n':
            if (strcmp(argv[1], "new") == 0) {
                if (argc != 3) {
                    print_err("Invalid, expects: catalyze new [name]");
                    arena_free(&arena);
                    return 1;
                }

                result = new_project(argv[2]);
                if (IS_ERR(result)) {
                    print_err(ERR_MSG(result));
                    arena_free(&arena);
                    return 1;
                }

                printf("\nCreated \e[1m%s\e[0m! Happy coding!\n", argv[2]);
            } else {
                print_help();
                arena_free(&arena);
                return 1;
            }
            break;

        case 'r':
            if (strcmp(argv[1], "run") == 0) {
                if (argc == 2) {
                    result = parse_config(&arena);

                    if (IS_ERR(result)) {
                        print_err(ERR_MSG(result));
                        arena_free(&arena);
                        return 1;
                    }

                    config = (CatalyzeConfig*) result.data; 
                    result = run_project_all(&arena, config);

                    if (IS_ERR(result)) {
                        print_err(ERR_MSG(result));
                        arena_free(&arena);
                        return 1;
                    }
                } else if (argc == 3) {
                    result = parse_config(&arena);

                    if (IS_ERR(result)) {
                        print_err(ERR_MSG(result));
                        arena_free(&arena);
                        return 1;
                    }

                    config = (CatalyzeConfig*) result.data; 
                    result = run_project_target(&arena, config, argv[2]);

                    if (IS_ERR(result)) {
                        print_err(ERR_MSG(result));
                        arena_free(&arena);
                        return 1;
                    }
                } else {
                    print_err("Invalid, expects: catalyze run [name] - Name is optional");
                    arena_free(&arena);
                    return 1;
                }
            } else {
                print_help();
                arena_free(&arena);
                return 1;
            }
            break;

        default:
            print_help();
            arena_free(&arena);
            return 1;
    }

    arena_free(&arena);
    return 0;
}
