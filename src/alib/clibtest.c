
// playground to try various functions offered by this library

#include "aio.h"
#include "autil.h"
#include "awindow.h"

void clibtestmain (void)
{
    int i;

    __aqb_line (1, 2, 3, 4, 255, 6);

    // window(1, "AQB Main Window", 0, 0, 619, 189, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);
    __aqb_window_open(1, "AQB Main Window", -1, -1, -1, -1, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);

    __aqb_line (30, 30, 50, 60,  AW_LINE_FLAG_BOX, 0);
    __aqb_line (130, 130, 150, 160, AW_LINE_FLAG_BOX | AW_LINE_FLAG_FILL, 0);
    __aqb_line (300, 10, 400, 100, 0, 0);
    __aqb_line (310, 10, 410, 100, 0, 0);

    _aio_puts("clibtestmain:\n");
    _aio_puts("puts4: "); _aio_puts4(12345678); _aio_putnl();
    _aio_puts("puts2: "); _aio_puts2(12345); _aio_putnl();

    for (i=0; i<100; i++)
    {
        // _aio_puts("i="); _aio_puts2(i); _aio_putnl();
        // __aqb_line (i, i*2, i*3, i*4, i*5, i*6);
        __aqb_line (i*5, 0, 320, 200, 0, 0);
    }

    delay(5);
}

