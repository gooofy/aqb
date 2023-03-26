#ifndef HAVE_GADTOOLS_SUPPORT_H
#define HAVE_GADTOOLS_SUPPORT_H

#include <exec/types.h>

#define MOD2_NUM 4

typedef struct
{
    ULONG ul1;
    SHORT s;
    ULONG ul2;
} mod2_t;

extern mod2_t    _g_m2[MOD2_NUM];

void _mod2_init(void);

#endif

