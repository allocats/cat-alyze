#ifndef CORE_H
#define CORE_H

#include <stdio.h>

static void write_config(FILE* fptr, const char* name) {
    fprintf(fptr, "config {\n\tcompiler: clang\n\tbuild_dir: build/\n\tdefault_flags: -Wall -Wextra\n}\n\ntarget executable %s {\n\tsources: src/main.c\n\tflags: -O3\n\toutput: build/bin/%s\n}\n\ntarget debug %s_debug {\n\tsources: src/main.c\n\tflags: -O0 -g3 -fsanitize=address -Weverything\n\toutput: build/debug/%s_debug\n}\n\ntarget test %s_tests {\n\tsources: tests/test_main.c\n\tflags: -g -Weverything\n\toutput: build/tests/%s_test\n}", name, name, name, name, name, name);
}

#endif // !CORE_H
