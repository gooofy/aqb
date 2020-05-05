
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
    _aio_puts("gadget1test\n");

    __aqb_window_open(1, "gadget1test", 10, 10, 320, 100, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);

    __aqb_on_window_call(wincb);

    //                   id, type, enabled, x, y, w, h, txt, shortcut, style
    __aqb_gadget_create (1, AW_GADGET_TYPE_ACTION, TRUE, 10, 10, 120, 25, "TEST", 'T', AW_GADGET_STYLE_1);

    __aqb_gadget_refresh();

    // wait for window to be closed

    while (TRUE)
    {
        __aqb_sleep();
        if (finished)
            break;
    }
}
