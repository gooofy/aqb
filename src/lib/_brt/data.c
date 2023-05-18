#define ENABLE_DPRINTF

#include "_brt.h"

#include <exec/types.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

// DATA / READ / RESTORE support

static void *g_data_ptr=NULL;
extern uint32_t *_data__end;

void _AQB_RESTORE (void *p)
{
    g_data_ptr = p;
}

static inline BOOL _check_out_of_data(void)
{
    return !g_data_ptr || ((intptr_t)g_data_ptr >= (intptr_t)&_data__end);
}

void _AQB_READ1 (void *v)
{
    if (_check_out_of_data())
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
    //DPRINTF ("_AQB_READ2: g_data_ptr=0x%08lx, _data__end=0x%08lx\n",
    //         g_data_ptr, _data__end);
    if (_check_out_of_data())
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    *((SHORT*) v) = *((SHORT *)g_data_ptr);

    g_data_ptr += 2;
}

void _AQB_READ4 (void *v)
{
    if (_check_out_of_data())
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
    if (_check_out_of_data())
    {
        ERROR (ERR_OUT_OF_DATA);
        return;
    }

    UBYTE c = 0xff;
    LONG l = 0;
    while (c && (l<MAX_STRING_LEN-1) && !_check_out_of_data())
    {
        c = buf[l] = *((char *)g_data_ptr);
        // _debug_puts("_aqb_readStr: c="); _debug_puts2(c); _debug_putnl();
        g_data_ptr += 1;
        l++;
    }
    buf[l] = 0;
    *((UBYTE **)v) = _astr_dup(buf);
}

