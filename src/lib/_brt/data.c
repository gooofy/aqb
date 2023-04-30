#include "_brt.h"

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

// DATA / READ / RESTORE support

static void *g_data_ptr=NULL;

void _AQB_RESTORE (void *p)
{
    g_data_ptr = p;
}

void _AQB_READ1 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    g_data_ptr += 1;    // skip stuffing byte

    *((BYTE*) v) = *((BYTE *)g_data_ptr);

    g_data_ptr += 1;
}

void _AQB_READ2 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((SHORT*) v) = *((SHORT *)g_data_ptr);

    g_data_ptr += 2;
}

void _AQB_READ4 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((LONG*) v) = *((LONG *)g_data_ptr);

    g_data_ptr += 4;
}

#define MAX_STRING_LEN 1024

void _AQB_READSTR (void *v)
{
    UBYTE buf[MAX_STRING_LEN];
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    UBYTE c = 0xff;
    LONG l = 0;
    while (c && (l<MAX_STRING_LEN-1))
    {
        c = buf[l] = *((char *)g_data_ptr);
        // _debug_puts("_aqb_readStr: c="); _debug_puts2(c); _debug_putnl();
        g_data_ptr += 1;
        l++;
    }
    buf[l] = 0;
    *((UBYTE **)v) = _astr_dup(buf);
}

