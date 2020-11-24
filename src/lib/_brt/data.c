#include "_brt.h"

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

// DATA / READ / RESTORE support

static void *g_data_ptr=NULL;

void _aqb_restore (void *p)
{
    g_data_ptr = p;
}

void _aqb_read1 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((BYTE*) v) = *((BYTE *)g_data_ptr);

    g_data_ptr += 1;
}

void _aqb_read2 (void *v)
{
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((SHORT*) v) = *((SHORT *)g_data_ptr);

    g_data_ptr += 2;
}

void _aqb_read4 (void *v)
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

void _aqb_readStr (void *v)
{
    char buf[MAX_STRING_LEN];
    if (!g_data_ptr)
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    char c = 0xff;
    int l = 0;
    while (c && (l<MAX_STRING_LEN-1))
    {
        c = buf[l] = *((char *)g_data_ptr);
        // _debug_puts("_aqb_readStr: c="); _debug_puts2(c); _debug_putnl();
        g_data_ptr += 1;
        l++;
    }
    buf[l] = 0;
    *((char **)v) = _astr_dup(buf);
}

