
#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#include <assert.h>
//#include <math.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <_brt/_brt.h>

ULONG _aqb_stack_size = 8192;

int _aqb_main (int argc, char *argv[])
{

    FLOAT f3 = SPFlt(31415)/SPFlt(10000);
    _debug_puts((STRPTR) "f3=");
    _debug_putf(f3);
    _debug_putnl();

    if (f3 > SPFlt(3))
        _debug_puts((STRPTR) ">3\n");

    return 0;
}

