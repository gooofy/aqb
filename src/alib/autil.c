
#include "autil.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>

USHORT g_errcode;

static struct Remember *g_rl = NULL;

APTR ralloc(ULONG size, ULONG flags)
{
    return AllocRemember(&g_rl, size, flags);
}

void autil_init(void)
{
}

void autil_shutdown(void)
{
    if (g_rl) FreeRemember(&g_rl, TRUE);
}

