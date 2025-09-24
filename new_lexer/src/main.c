#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lexer.h"

#include "whisker/macros.h"

int main(int argc, char* argv[]) {
    int fd = open("test", O_RDONLY);
    if (UNLIKELY(fd == -1)) {
        fprintf(stderr, "File not found!\n");
        return 1;
    } 

    struct stat st;
    if (UNLIKELY(fstat(fd, &st) == -1)) {
        close(fd);
        fprintf(stderr, "Stat failed!\n");
        return 1;
    }

    size_t padded_len = ((st.st_size + 63) / 64) * 64 + 64;

    // Freed inside of destroy_lexer()
    char* buffer = aligned_alloc(64, padded_len); 
    if (UNLIKELY(!buffer)) {
        close(fd);
        fprintf(stderr, "Allocation failed!\n");
        return 1;
    }

    ssize_t n = read(fd, buffer, st.st_size);
    if (UNLIKELY(n != (ssize_t) st.st_size)) {
        close(fd);
        free(buffer);
        fprintf(stderr, "Read failed!\n");
        return 1;
    }

    close(fd);
    memset(buffer + st.st_size, ' ', padded_len - st.st_size);
    buffer[padded_len - 1] = 0;

    Lexer* lexer = create_lexer(buffer, padded_len);
    lex(lexer);
    destroy_lexer(lexer);
}
