
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

static BOOL finished = FALSE;

static void wincb(void)
{
    _aio_puts("win callback\n");
    finished = TRUE;
}

void clibtestmain (void)
{
    _aio_puts("win1test\n");

    // window(1, "AQB Main Window", 0, 0, 619, 189, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);
    __aqb_window_open(1, "win1test", -1, -1, -1, -1, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);

    __aqb_on_window_call(wincb);

    // __aqb_line (30, 30, 50, 60,  AW_LINE_FLAG_BOX, 0);
    // __aqb_line (130, 130, 150, 160, AW_LINE_FLAG_BOX | AW_LINE_FLAG_FILL, 0);
    // __aqb_line (300, 10, 400, 100, 0, 0);
    // __aqb_line (310, 10, 410, 100, 0, 0);

    for (int i=0; i<100; i++)
    {
        // _aio_puts("i="); _aio_puts2(i); _aio_putnl();
        // __aqb_line (i, i*2, i*3, i*4, i*5, i*6);
        __aqb_line (i*5, 0, 320, 200, 0, 0);
    }

    // wait for window to be closed

    _aio_puts("sleep...\n");

    while (TRUE)
    {
        __aqb_sleep();
        if (finished)
            break;
    }
}
