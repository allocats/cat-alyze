#!/usr/bin/env bash

rm -rvf build &>/dev/null
mkdir -p build/bin &>/dev/null 

clang -g3 -ggdb3 -c src/main.c -o build/main.o 
clang -g3 -ggdb3 -c src/config/config.c -o build/config.o 
clang -g3 -ggdb3 -c src/config/lexer.c -o build/lexer.o 
clang -g3 -ggdb3 -c src/core/build.c -o build/build.o 
clang -g3 -ggdb3 -c src/core/debug.c -o build/debug.o 
clang -g3 -ggdb3 -c src/core/new.c -o build/new.o 
clang -g3 -ggdb3 -c src/core/init.c -o build/init.o 
clang -g3 -ggdb3 -c src/core/run.c -o build/run.o 

clang -g3 -ggdb3 build/main.o build/config.o build/lexer.o build/build.o build/new.o build/init.o build/run.o build/debug.o -o build/bin/catalyze
