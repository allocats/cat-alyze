#ifndef MACROS_H
#define MACROS_H

#include <stdlib.h>

#define UNLIKELY(x) __builtin_expect(x, 0)
#define LIKELY(x) __builtin_expect(x, 1)

#define W_UNSUED(x) (void)(x)
#define W_TODO(msg) fprintf(stdout, "%s:%d: \e[1mTODO\e[0m: %s\n", __FILE__, __LINE__, msg)
#define W_UNREACHABLE(msg) do { fprintf(stderr, "%s:%d: \e[1mUNREACHABLE\e[0m: %s\n", __FILE__, __LINE__, msg); abort();} while(0)

#endif // !MACROS_H
