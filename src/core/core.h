#ifndef CORE_H
#define CORE_H

#include <stdio.h>

static void write_config(FILE* fptr, const char* name) {
    fprintf(fptr, "config {\n\tcompiler: clang\n\tbuild_dir: build/\n\tdefault_flags: -Wall -Wextra\n}\n\ntarget executable %s {\n\tsources: src/main.c\n\tflags: -O3\n\toutput: build/bin/%s\n}\n\ntarget debug %s_debug {\n\tsources: src/main.c\n\tflags: -O0 -g3 -fsanitize=address -fno-omit-frame-pointer\n\toutput: build/bin/%s_debug\n}", name, name, name, name);
}

#endif // !CORE_H
