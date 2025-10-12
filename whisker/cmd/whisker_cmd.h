#ifndef  WHISKER_CMD_H
#define  WHISKER_CMD_H

#ifdef WHISKER_NOPREFIX
    #define cmd_append whisker_cmd_append
    #define cmd_execute whisker_cmd_execute
    #define cmd_destroy whisker_cmd_destroy
    #define cmd_print whisker_cmd_print
#endif

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    const char** items;
    size_t count;
    size_t capacity;
} Whisker_Cmd;

bool whisker_cmd_append(Whisker_Cmd* cmd, const char* arg);
bool whisker_cmd_execute(Whisker_Cmd* cmd);
void whisker_cmd_destroy(Whisker_Cmd* cmd);

void whisker_cmd_print(Whisker_Cmd* cmd);

#endif // ! WHISKER_CMD_H
