#include "whisker_cmd.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

bool whisker_cmd_append(Whisker_Cmd* cmd, const char* arg) {
    assert(cmd);

    if (cmd -> count >= cmd -> capacity) {
        const size_t new_capacity = cmd -> capacity == 0 ? 8 : cmd -> capacity * 2;
        if (new_capacity < cmd -> capacity || new_capacity > (SIZE_MAX / sizeof(const char*))) {
            return false;
        }

        const char** new_items = realloc(cmd -> items, sizeof(const char*) * new_capacity);
        if (!new_items) {
            return false;
        }

        cmd -> items = new_items;
        cmd -> capacity = new_capacity;
    }

    cmd -> items[cmd -> count++] = arg;
    return true;
}

bool whisker_cmd_execute(Whisker_Cmd* cmd) {
    assert(cmd && cmd -> count > 0);
    
    if (cmd -> items[cmd -> count - 1] != NULL) {
        if (!whisker_cmd_append(cmd, NULL)) {
            return false;
        }
    }

    pid_t pid = vfork();
    if (pid < 0) {
        return false;
    }

    if (pid == 0) {
        execvp(cmd -> items[0], (char* const*) cmd -> items);
        fprintf(stderr, "execvp failed! %s\n", cmd -> items[0]);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        return false;
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

void whisker_cmd_destroy(Whisker_Cmd* cmd) {
    if (!cmd) return;

    free(cmd -> items);
    cmd -> items = NULL;
    cmd -> count = 0;
    cmd -> capacity= 0;
}

void whisker_cmd_print(Whisker_Cmd* cmd) {
    printf("CMD:");

    size_t count = cmd -> count - 1; // subtract 1 as the final arg has to be null for exevp()
    for (size_t i = 0; i < count; i++) {
        printf(" %s", cmd -> items[i]);
    } 

    printf("\n");
}
