#define __NO_INLINE__
#include <string.h>
__stdargs void *memmove(void *s1, const void *s2, size_t n) {
    extern __stdargs void bcopy();

    bcopy(s2, s1, n);
    return s1;
}
