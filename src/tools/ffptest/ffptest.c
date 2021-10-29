
#include <stdio.h>
//#include <stdlib.h>
//#include <stdarg.h>
//#include <string.h>
//#include <errno.h>
#include <assert.h>
//#include <math.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/mathtrans_protos.h>
#include <inline/mathtrans.h>

#include <_brt/_brt.h>

int _aqb_main (int argc, char *argv[])
{
    FLOAT f1 = SPFlt(10);
    FLOAT f2 = SPFlt(42);

    FLOAT f3 = f1*f2;
    FLOAT f4 = f2/f1;

    DPRINTF ("f1=%d, f2=%d, f3=%d, f4=f2/f1=%d\n", SPFix(f1), SPFix(f2), SPFix(f3), SPFix(f4));

    BOOL b = f1 < f2;

    DPRINTF ("b = f1 < f2 = %d\n", b);

    FLOAT f5 = f2 - f1;

    DPRINTF ("f5 = f2 - f1 = %d\n", SPFix(f5));

    return 0;
}

