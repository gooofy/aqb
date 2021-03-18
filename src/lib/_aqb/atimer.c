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

static FLOAT g_1000;

ULONG _g_timer_signals = 0;

void _atimer_init(void)
{
    g_1000 = SPFlt(1000);
}

void _atimer_shutdown(void)
{
    // FIXME
}

void _atimer_process_signals(void)
{
    // FIXME
	_debug_puts ((UBYTE*)"_atimer_process_signals"); _debug_putnl();
}

void ON_TIMER_CALL (SHORT id, FLOAT d, void (*cb)(void))
{
	_debug_puts ((UBYTE*)"ON_TIMER_CALL #"); _debug_puts2(id); _debug_putnl();
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
    usecs = SPFix (SPMul (g_1000, SPSub (SPFlt(secs), d)));

    _debug_puts ((UBYTE*)"secs="); _debug_putu4(secs); _debug_putnl();
    _debug_puts ((UBYTE*)"usecs="); _debug_putu4(usecs); _debug_putnl();

    g_timers[id-1].cb          = cb;
    g_timers[id-1].tv.tv_sec   = secs;
    g_timers[id-1].tv.tv_usec  = usecs;
    g_timers[id-1].timerport   = NULL;
    g_timers[id-1].timer_io    = NULL;
}

void TIMER_ON (SHORT id)
{
	_debug_puts ((UBYTE*)"TIMER_ON #"); _debug_puts2(id); _debug_putnl();

    if ( (id < 1) || (id > MAX_NUM_TIMERS) || !g_timers[id-1].cb )
    {
        ERROR(AE_TIMER_ON);
        return;
    }

    if (g_timers[id-1].timer_io)    // if this timer is already on, we do not throw an error
        return;

    g_timers[id-1].timerport = CreatePort( 0, 0 );
	if (timerport == NULL)
	{
        ERROR(AE_TIMER_ON);
		return;
	}

    g_timers[id-1].timer_io = (struct timerequest *) CreateExtIO( g_timers[id-1].timerport, sizeof( struct timerequest ) );
	if (g_timers[id-1].timer_io == NULL)
    {
        TIMER_OFF(id);
        ERROR(AE_TIMER_ON);
		return;
    }

	LONG error = OpenDevice( TIMERNAME, UNIT_MICROHZ, (struct IORequest *) g_timers[id-1].timer_io, 0 );
	if (error)
	{
        TIMER_OFF(id);
        ERROR(AE_TIMER_ON);
		return;
	}

    g_timers[id-1].timer_io->tr_node.io_Command = TR_ADDREQUEST;
    g_timers[id-1].timer_io->tr_node.tr_time    = g_timers[id-1].tv;

}

void TIMER_OFF (SHORT id)
{
	_debug_puts ((UBYTE*)"TIMER_OFF #"); _debug_puts2(id); _debug_putnl();
    // FIXME
		//DeletePort(g_timers[id-1].timerport);
		//delete_timer( TimerIO );
}

