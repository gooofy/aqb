
// playground to try various functions offered by this library

#include "io.h"
#include "autil.h"
#include "awindow.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <dos/dosextens.h>
#include <dos/dostags.h>
#include <dos/rdargs.h>

#include <exec/execbase.h>
#include <exec/memory.h>

#include <clib/utility_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>


struct IntuitionBase *IntuitionBase;

void clibtestmain (void)
{
    int i;

    IntuitionBase = (struct IntuitionBase *) OpenLibrary((CONST_STRPTR)"intuition.library",0);
    if (!IntuitionBase)
    {
        aputs("*** error: failed to open intuition.library!\n");
        goto shutdown;
    }

    autil_init();

    AW_open(1, "AQB Main Window", 0, 0, 619, 189, AW_FLAG_SIZE | AW_FLAG_DRAG | AW_FLAG_DEPTH | AW_FLAG_CLOSE | AW_FLAG_REFRESH, 0);

    Delay(250);

    aputs("clibtestmain:"); putnl();
    aputs("puts4: "); puts4(12345678); putnl();
    aputs("puts2: "); puts2(12345); putnl();

    for (i=0; i<10; i++)
    {
        aputs("i="); puts2(i); putnl();
    }


shutdown:

    AW_shutdown();
    autil_shutdown();

    if (IntuitionBase) 
        CloseLibrary( (struct Library *)IntuitionBase);
}

