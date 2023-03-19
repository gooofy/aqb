#define __NO_INLINE__
#include <proto/exec.h>
#include <string.h>

void *memcpy(void *s1,const void *s2,size_t n)
{
    CopyMem((APTR)s2,s1,n); return s1;
}
