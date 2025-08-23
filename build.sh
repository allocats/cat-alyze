#!/usr/bin/env bash

rm -rvf build
mkdir -p build/bin 

clang -g3 -ggdb3 -c src/main.c -o build/main.o 
clang -g3 -ggdb3 -c src/config/config.c -o build/config.o 
clang -g3 -ggdb3 -c src/config/lexer.c -o build/lexer.o 
clang -g3 -ggdb3 -c src/core/build.c -o build/build.o 
clang -g3 -ggdb3 build/main.o build/config.o build/lexer.o build/build.o -o build/bin/catalyze

./build/bin/catalyze
