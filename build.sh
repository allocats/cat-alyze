#!/usr/bin/env bash

rm -rvf build &>/dev/null
mkdir -p build/bin &>/dev/null 

clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -mavx2 -c src/utils/arena.c -o build/arena.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/config/config.c -o build/config.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/config/lexer.c -o build/lexer.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/core/build.c -o build/build.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/core/debug.c -o build/debug.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/core/new.c -o build/new.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/core/init.c -o build/init.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/core/run.c -o build/run.o 
clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -c src/main.c -o build/main.o 

clang -Wall -Wextra -Werror -pedantic -O3 -flto -march=native -mavx2 build/arena.o build/main.o build/config.o build/lexer.o build/build.o build/new.o build/init.o build/run.o build/debug.o -o build/bin/catalyze
