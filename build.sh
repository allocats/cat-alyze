#!/usr/bin/env bash

rm -rvf build
mkdir -p build/bin 

clang -c src/main.c -o build/main.o 
clang -c src/config/config.c -o build/config.o 
clang -c src/config/lexer.c -o build/lexer.o 
clang build/main.o build/config.o build/lexer.o -o build/bin/catalyze

./build/bin/catalyze
