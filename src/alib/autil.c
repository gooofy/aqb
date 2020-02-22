
#include "autil.h"
#include "aio.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>
#include <clib/dos_protos.h>

USHORT g_errcode;

static struct Remember *g_rl = NULL;

APTR _autil_alloc(ULONG size, ULONG flags)
{
    return AllocRemember(&g_rl, size, flags);
}

void _autil_init(void)
{
}

void _autil_shutdown(void)
{
    if (g_rl) FreeRemember(&g_rl, TRUE);
}

// BASIC: DELAY <seconds>
void delay(ULONG seconds)
{
    Delay(seconds * 50L);
}

