#include "new.h"

#include "../config/config.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void create_dir(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char cmd[size];
    size_t offset = snprintf(cmd, size, "mkdir -p ");
    offset += snprintf(cmd + offset, size - offset, "%s/src", name);

    system(cmd);
}

static Result create_config(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char cmd[size];
    size_t offset = snprintf(cmd, size, "touch ");
    offset += snprintf(cmd + offset, size - offset, "%s/config.cat", name);

    system(cmd);

    char path[size];
    snprintf(path, size, "%s/config.cat", name);

    FILE* fptr = fopen(path, "w");
    if (fptr == NULL) {
        fclose(fptr);
        return err("Failed to write to config.cat");
    }

    fprintf(fptr, "config {\n\tcompiler: clang\n\tbuild_dir: build/\n\tdefault_flags: -Wall -Wextra\n}\n\ntarget executable %s {\n\tsources: src/main.c\n\tflags: -O3\n\toutput: build/bin/%s\n}", name, name);

    fclose(fptr);
    return ok(NULL);
}

static Result create_main(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char cmd[size];
    size_t offset = snprintf(cmd, size, "touch ");
    offset += snprintf(cmd + offset, size - offset, "%s/src/main.c", name);

    system(cmd);

    char path[size];
    snprintf(path, size, "%s/src/main.c", name);

    FILE* fptr = fopen(path, "w");
    if (fptr == NULL) {
        fclose(fptr);
        return err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");

    fclose(fptr);
    return ok(NULL);
} 

Result new_project(const char* name) {
    if (strlen(name) > MAX_NAME_LEN) {
        return err("Project name too long");
    }

    create_dir(name);
    create_config(name);
    create_main(name);

    return ok(NULL);
}
