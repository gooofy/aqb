#define __NO_INLINE__
#include <string.h>
__stdargs size_t strlen(const char *string) {
    const char *s = string;

    while (*s++) {
    }
    return ~(string - s);
}
