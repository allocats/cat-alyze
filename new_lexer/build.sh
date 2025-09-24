mkdir -p build/bin/

nasm -g -f elf64 -F dwarf src/whisker/stringd/ws_strnlen.asm -o build/ws_strnlen.o

clang -O0 -g -mavx2 build/ws_strnlen.o src/lexer.c src/main.c -o build/bin/lexer
