#!/usr/bin/env bash

rm -rvf build &>/dev/null
mkdir -p build/bin &>/dev/null 

clang -O3 -march=native -flto=thin -c src/main.c -o build/main.o 
clang -O3 -march=native -flto=thin -c src/config/config.c -o build/config.o 
clang -O3 -march=native -flto=thin -c src/config/lexer.c -o build/lexer.o 
clang -O3 -march=native -flto=thin -c src/core/build.c -o build/build.o 
clang -O3 -march=native -flto=thin -c src/core/debug.c -o build/debug.o 
clang -O3 -march=native -flto=thin -c src/core/new.c -o build/new.o 
clang -O3 -march=native -flto=thin -c src/core/init.c -o build/init.o 
clang -O3 -march=native -flto=thin -c src/core/run.c -o build/run.o 

clang -O3 -march=native -flto=thin build/main.o build/config.o build/lexer.o build/build.o build/new.o build/init.o build/run.o build/debug.o -o build/bin/catalyze
