#ifndef MACROS_H
#define MACROS_H

#define UNLIKELY(x) __builtin_expect(x, 0)
#define LIKELY(x) __builtin_expect(x, 1)

#endif // !MACROS_H
