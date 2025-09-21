#!/usr/bin/env bash

rm -rvf build &>/dev/null
mkdir -p build/bin &>/dev/null 

clang -O2 -mavx2 -march=native -flto -c src/utils/arena.c -o build/arena.o 
clang -O2 -mavx2 -march=native -flto -c src/config/config.c -o build/config.o 
clang -O2 -mavx2 -march=native -flto -c src/config/lexer.c -o build/lexer.o 
clang -O2 -mavx2 -march=native -flto -c src/core/build.c -o build/build.o 
clang -O2 -mavx2 -march=native -flto -c src/core/debug.c -o build/debug.o 
clang -O2 -mavx2 -march=native -flto -c src/core/new.c -o build/new.o 
clang -O2 -mavx2 -march=native -flto -c src/core/init.c -o build/init.o 
clang -O2 -mavx2 -march=native -flto -c src/core/run.c -o build/run.o 
clang -O2 -mavx2 -march=native -flto -c src/main.c -o build/main.o 

clang -O2 -mavx2 -march=native -flto build/arena.o build/main.o build/config.o build/lexer.o build/build.o build/new.o build/init.o build/run.o build/debug.o -o build/bin/catalyze
