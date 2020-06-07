
#include "autil.h"
#include "aio.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <clib/intuition_protos.h>
#include <clib/dos_protos.h>
#include <clib/mathffp_protos.h>

USHORT g_errcode;

static struct Remember *g_rl = NULL;

APTR __aqb_allocate(ULONG size, ULONG flags)
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

void __aqb_assert (BOOL b, const char *msg)
{
    if (b)
        return;

    _aio_puts(msg);
    _aio_puts("\n");

    _autil_exit(20);
}

static void (*error_handler)(void) = NULL;
static BOOL do_resume = FALSE;

SHORT _AQB_ERR=0;

void __aqb_on_error_call(void (*cb)(void))
{
    error_handler = cb;
}

void __aqb_error (SHORT errcode)
{
    do_resume = FALSE;
    _AQB_ERR = errcode;

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
        _AQB_ERR=0;
}

void __aqb_resume_next(void)
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

