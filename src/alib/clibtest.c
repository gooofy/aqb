
// playground to try various functions offered by this library

#include "aio.h"
#include "autil.h"
#include "awindow.h"

void clibtestmain (void)
{
    int i;

    window(1, "AQB Main Window", 0, 0, 619, 189, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);

    delay(5);

    _aio_puts("clibtestmain:\n");
    _aio_puts("puts4: "); _aio_puts4(12345678); _aio_putnl();
    _aio_puts("puts2: "); _aio_puts2(12345); _aio_putnl();

    for (i=0; i<10; i++)
    {
        _aio_puts("i="); _aio_puts2(i); _aio_putnl();
    }
}

