#!/usr/bin/env bash

mkdir -p build/bin/

clang -O3 -flto -c arena.c -o build/arena.o
clang -O3 -flto -mavx2 -c arena_avx2.c -o build/arena_avx2.o
clang -O3 -flto -msse2 -c arena_sse2.c -o build/arena_sse2.o

ar rcs build/bin/libarena.a build/arena.o build/arena_avx2.o build/arena_sse2.o
