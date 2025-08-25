#include "init.h"

#include "../config/config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void create_dir() {
    size_t size = 32;
    char cmd[size];
    snprintf(cmd, size, "mkdir -p src");

    system(cmd);
}

static Result create_config(const char* name) {
    FILE* fptr = fopen("config.cat", "w");
    if (fptr == NULL) {
        fclose(fptr);
        return err("Failed to write to config.cat");
    }

    fprintf(fptr, "config {\n\tcompiler: clang\n\tbuild_dir: build/\n\tdefault_flags: -Wall -Wextra\n}\n\ntarget executable %s {\n\tsources: src/main.c\n\tflags: -O3\n\toutput: build/bin/%s\n}", name, name);

    fclose(fptr);
    return ok(NULL);
}

static Result create_main() {
    FILE* fptr = fopen("src/main.c", "w");
    if (fptr == NULL) {
        fclose(fptr);
        return err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");

    fclose(fptr);
    return ok(NULL);
} 

Result init_project() {
    char cwd[MAX_NAME_LEN];
    getcwd(cwd, MAX_NAME_LEN);

    const char* name = strrchr(cwd, '/');
    name = name ? name + 1 : cwd;

    create_dir();
    create_config(name);
    create_main();

    return ok(NULL);
}
