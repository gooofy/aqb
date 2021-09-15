#include "_aqb.h"
#include "../_brt/_brt.h"

#define __USE_OLD_TIMEVAL__

#include <stdarg.h>

#include <exec/memory.h>

#include <devices/timer.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#define MAX_NUM_TIMERS 8

// #define ENABLE_DEBUG

typedef struct
{
	void (*cb)(void);
	struct timeval      tv;
	struct MsgPort     *timerport;
	struct timerequest *timer_io;
} atimer_t;

static atimer_t g_timers[MAX_NUM_TIMERS] =  {
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL },
												{ NULL, { {0}, {0} }, NULL, NULL }
											};

static FLOAT g_1e6;

ULONG _g_signalmask_atimer = 0;

void _atimer_init(void)
{
    g_1e6 = SPFlt(1000000);
}

void _atimer_shutdown(void)
{
    for (int i=0; i<MAX_NUM_TIMERS; i++)
        TIMER_OFF(i+1);
}

void _atimer_process_signals(ULONG signals)
{
#ifdef ENABLE_DEBUG
	_aio_puts ((UBYTE*)"_atimer_process_signals signals="); _aio_putu4(signals); _aio_putnl();
#endif

    for (int i=0; i<MAX_NUM_TIMERS; i++)
    {
        atimer_t *t = &g_timers[i];
        if (!t->cb)
            continue;

        if (!(signals & (1L << t->timerport->mp_SigBit)) )
            continue;

#ifdef ENABLE_DEBUG
        _aio_puts ((UBYTE*)"   --> timer #"); _aio_puts2(i+1); _aio_putnl();
#endif

        t->cb();

        if (g_timers[i].timer_io)
        {
            g_timers[i].timer_io->tr_node.io_Command = TR_ADDREQUEST;
            g_timers[i].timer_io->tr_time            = g_timers[i].tv;
            SendIO((struct IORequest *)g_timers[i].timer_io);
        }
    }
}

void ON_TIMER_CALL (SHORT id, FLOAT d, void (*cb)(void))
{
#ifdef ENABLE_DEBUG
	_aio_puts ((UBYTE*)"ON_TIMER_CALL #"); _aio_puts2(id); _aio_putnl();
#endif
    // error checking
    if ( (id < 1) || (id > MAX_NUM_TIMERS) )
    {
        ERROR(AE_ON_TIMER_CALL);
        return;
    }

	if (g_timers[id-1].cb)
	{
		TIMER_OFF(id);
	}

    ULONG secs, usecs;
    secs = SPFix (d);
    usecs = SPFix (SPMul (g_1e6, SPSub (SPFlt(secs), d)));

#ifdef ENABLE_DEBUG
    _aio_puts ((UBYTE*)"secs="); _aio_putu4(secs); _aio_putnl();
    _aio_puts ((UBYTE*)"usecs="); _aio_putu4(usecs); _aio_putnl();
#endif

    g_timers[id-1].cb          = cb;
    g_timers[id-1].tv.tv_sec   = secs;
    g_timers[id-1].tv.tv_usec  = usecs;
    g_timers[id-1].timerport   = NULL;
    g_timers[id-1].timer_io    = NULL;
}

void TIMER_ON (SHORT id)
{
#ifdef ENABLE_DEBUG
	_aio_puts ((UBYTE*)"TIMER_ON #"); _aio_puts2(id); _aio_putnl();
#endif

    if ( (id < 1) || (id > MAX_NUM_TIMERS) || !g_timers[id-1].cb )
    {
        ERROR(AE_TIMER_ON);
        return;
    }

    if (g_timers[id-1].timer_io)    // if this timer is already on, we do not throw an error
        return;

    g_timers[id-1].timerport = _autil_create_port (/*name=*/NULL, /*pri=*/0 );
	if (g_timers[id-1].timerport == NULL)
	{
        ERROR(AE_TIMER_ON);
		return;
	}

    g_timers[id-1].timer_io = (struct timerequest *) _autil_create_ext_io( g_timers[id-1].timerport, sizeof( struct timerequest ) );
	if (g_timers[id-1].timer_io == NULL)
    {
        TIMER_OFF(id);
        ERROR(AE_TIMER_ON);
		return;
    }

	LONG error = OpenDevice( (UBYTE *)TIMERNAME, UNIT_MICROHZ, (struct IORequest *) g_timers[id-1].timer_io, 0 );
	if (error)
	{
        TIMER_OFF(id);
        ERROR(AE_TIMER_ON);
		return;
	}

    g_timers[id-1].timer_io->tr_node.io_Command = TR_ADDREQUEST;
    g_timers[id-1].timer_io->tr_time            = g_timers[id-1].tv;

    _g_signalmask_atimer |= (1L << g_timers[id-1].timerport->mp_SigBit);
    SendIO((struct IORequest *)g_timers[id-1].timer_io);
}

void TIMER_OFF (SHORT id)
{
#ifdef ENABLE_DEBUG
	_aio_puts ((UBYTE*)"TIMER_OFF #"); _aio_puts2(id); _aio_putnl();
#endif
    if ( (id < 1) || (id > MAX_NUM_TIMERS) )
    {
        ERROR(AE_TIMER_OFF);
        return;
    }

    if (g_timers[id-1].timerport)
    {
        _autil_delete_port(g_timers[id-1].timerport);
        g_timers[id-1].timerport = NULL;
    }
    if (g_timers[id-1].timer_io)
    {
        AbortIO( (struct IORequest *) g_timers[id-1].timer_io );
        CloseDevice( (struct IORequest *) g_timers[id-1].timer_io );
        _autil_delete_ext_io ( (struct IORequest *) g_timers[id-1].timer_io );
        g_timers[id-1].timer_io = NULL;
    }
}
