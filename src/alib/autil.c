
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

void assertTrue (BOOL b, const char *msg)
{
    if (b)
        return;

    _aio_puts("ASSERTION FAILED: ");
    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

void assertEqualsInt (SHORT a, SHORT b, const char *msg)
{
    if (a == b)
        return;

    _aio_puts("ASSERTION FAILED ");
    _aio_puts2(a);
    _aio_puts(" != ");
    _aio_puts2(b);
    _aio_puts(": ");
    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

void assertEqualsLong (LONG  a, LONG  b, const char *msg)
{
    if (a == b)
        return;

    _aio_puts("ASSERTION FAILED ");
    _aio_puts4(a);
    _aio_puts(" != ");
    _aio_puts4(b);
    _aio_puts(": ");
    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

void assertEqualsSingle (FLOAT a, FLOAT b, const char *msg)
{
    LONG la, lb;

    la = *((LONG *) &a);
    lb = *((LONG *) &b);

    if (la == lb)
        return;

    _aio_puts("ASSERTION FAILED ");
    _aio_putf(a);
    _aio_puts(" != ");
    _aio_putf(b);
    _aio_puts(": ");
    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

