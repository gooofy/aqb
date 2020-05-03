
// test window event handling

#include "aio.h"
#include "autil.h"
#include "astr.h"
#include "awindow.h"

#include <clib/mathffp_protos.h>
#include <clib/mathtrans_protos.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>

void clibtestmain (void)
{
    FLOAT t;

    _aio_puts("timer1test\n");

    for (int i=0; i<5; i++)
    {
        _aio_puts("timer: ");
        t = __aqb_timer_fn();
        _aio_putf(t);
        _aio_puts4(*((LONG*) &t));
        _aio_puts("\n");
        delay(1);
    }
}
