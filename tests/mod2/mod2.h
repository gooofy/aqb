#ifndef HAVE_GADTOOLS_SUPPORT_H
#define HAVE_GADTOOLS_SUPPORT_H

#include <exec/types.h>

#define MOD2_NUM 8

typedef struct
{
    SHORT s;
    BOOL  b;
} mod2_t;

extern mod2_t    _g_m2[MOD2_NUM];

#endif

