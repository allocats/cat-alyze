#ifndef RESULT_H
#define RESULT_H

#include <string.h>

typedef enum {
    OK,
    ERR
} ResultType;

typedef struct {
    ResultType type;
    union {
        void* data;
        char message[128];
    };
} Result;

static inline Result ok(void* data) {
    Result res = {.type = OK, .data= data};
    return res;
}

static inline Result err(const char* msg) {
    Result res = {.type = ERR};

    if (msg) {
        strncpy(res.message, msg, sizeof(res.message) - 1);
    }

    res.message[sizeof(res.message) - 1] = '\0';
    return res;
}

#define IS_OK(result) ((result).type == OK)
#define IS_ERR(result) ((result).type == ERR)
#define ERR_MSG(result) ((result).message)

#endif // !RESULT_H 
