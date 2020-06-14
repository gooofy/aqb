
// test pointer casting

#include "aio.h"
#include "autil.h"
#include "astr.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>

void clibtestmain (void)
{
    int    i;
    int   *ip;
    short  b;
    short *bp;

    _aio_puts("ptr1test\n");

    i = 0x0080;
    b = (short) i;

    ip = &i;
    bp = (short *) ip;

    _aio_puts2(i);
    _aio_puttab();
    _aio_puts1(b);
    _aio_putnl();

    _aio_puts2(*ip);
    _aio_puttab();
    _aio_puts1(*bp);
    _aio_putnl();

}
