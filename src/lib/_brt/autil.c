
#include "_brt.h"

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/mathffp_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/mathffp.h>

USHORT g_errcode;

// not using Intuition's AllocRemember here because we want minimal dependencies for the AQB core module

typedef struct AQB_memrec_ *AQB_memrec;

struct AQB_memrec_
{
    AQB_memrec next;
    ULONG      size;
    APTR      *mem;
};

static AQB_memrec g_mem = NULL;

APTR allocate_(ULONG size, ULONG flags)
{
    AQB_memrec mem_prev = g_mem;

    g_mem = (AQB_memrec) AllocMem (sizeof(*g_mem), 0);
    if (!g_mem)
    {
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->mem = (APTR) AllocMem (size, flags);
    if (!g_mem->mem)
    {
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->size = size;
    g_mem->next = mem_prev;

    return g_mem->mem;
}

void _autil_init(void)
{
}

void _autil_shutdown(void)
{
    while (g_mem)
    {
        AQB_memrec mem_next = g_mem->next;
        FreeMem(g_mem->mem, g_mem->size);
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_next;
    }
}

void _aqb_assert (BOOL b, const char *msg)
{
    if (b)
        return;

    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

static void (*error_handler)(void) = NULL;
static BOOL do_resume = FALSE;

SHORT ERR=0;

void _aqb_on_error_call(void (*cb)(void))
{
    error_handler = cb;
}

void _aqb_error (SHORT errcode)
{
    do_resume = FALSE;
    ERR = errcode;

    if (error_handler)
    {
        error_handler();
    }
    else
    {
        _aio_puts("*** unhandled runtime error code: "); _aio_puts2(errcode);
        _aio_puts("\n");
    }

    if (!do_resume)
        _autil_exit(errcode);
    else
        ERR=0;
}

void _aqb_resume_next(void)
{
    do_resume = TRUE;
}

FLOAT __aqb_timer_fn (void)
{
	FLOAT res;

	struct DateStamp datetime;

	DateStamp(&datetime);

    // _aio_puts4(datetime.ds_Days);
    // _aio_puts4(datetime.ds_Minute);
    // _aio_puts4(datetime.ds_Tick);

	res = SPFlt(datetime.ds_Minute);
    // _aio_putf(res);
	res = SPAdd(SPMul(res, SPFlt(60)), SPDiv(SPFlt(datetime.ds_Tick), SPFlt(50)));
    // _aio_putf(res);
    // _aio_puts("\n");

	return res;
}

