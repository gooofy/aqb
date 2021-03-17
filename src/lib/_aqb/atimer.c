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
	struct timeval      currentval;
	struct MsgPort     *timerport;
	struct timerequest *TimerIO;
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
void _atimer_init(void)
{
    // FIXME
}

void _atimer_shutdown(void)
{
    // FIXME
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

    // FIXME
}

void TIMER_ON (SHORT id)
{

	_debug_puts ((UBYTE*)"TIMER_ON #"); _debug_puts2(id); _debug_putnl();

	// FIXME
#if 0

	LONG error;
	struct MsgPort *timerport;
	struct timerequest *TimerIO;

	timerport = CreatePort( 0, 0 );
	if (timerport == NULL)
	{
        ERROR(AE_SCREEN_OPEN);
		return( NULL );
	}

	TimerIO = (struct timerequest *) CreateExtIO( timerport, sizeof( struct timerequest ) );
	if (TimerIO == NULL )
		{
		DeletePort(timerport);   /* Delete message port */
		return( NULL );
		}

	error = OpenDevice( TIMERNAME, unit, (struct IORequest *) TimerIO, 0 );
	if (error != 0 )
	{
		delete_timer( TimerIO );
		return( NULL );
	}
	return( TimerIO );
#endif
}

void TIMER_OFF (SHORT id)
{
	_debug_puts ((UBYTE*)"TIMER_OFF #"); _debug_puts2(id); _debug_putnl();
    // FIXME
}

