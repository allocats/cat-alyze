#!/usr/bin/env bash

rm -rvf build &>/dev/null
mkdir -p build/bin &>/dev/null 

CFLAGS="-Wall -Wextra -Werror -pedantic -O3 -flto -march=native"

clang $CFLAGS -c src/config/config.c -o build/config.o
clang $CFLAGS -c src/config/lexer.c -o build/lexer.o
clang $CFLAGS -c src/core/build.c -o build/build.o
clang $CFLAGS -c src/core/debug.c -o build/debug.o
clang $CFLAGS -c src/core/new.c -o build/new.o
clang $CFLAGS -c src/core/init.c -o build/init.o
clang $CFLAGS -c src/core/run.c -o build/run.o
clang $CFLAGS -c src/main.c -o build/main.o

clang $CFLAGS build/main.o build/config.o build/lexer.o build/build.o build/new.o build/init.o build/run.o build/debug.o \
    src/lib/libarena.a -o build/bin/catalyze \
