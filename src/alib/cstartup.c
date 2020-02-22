/*
 * C part of AQB startup
 *
 * opens libraries, initializes other modules,
 * calls __aqbmain
 * and shuts down everything once __aqbmain returns
 */

#include "aio.h"
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

struct DOSBase       *DOSBase       = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase       *GfxBase       = NULL;

static BOOL awindow_init_done = FALSE;
static BOOL autil_init_done   = FALSE;
static BOOL aio_init_done     = FALSE;

static void _cshutdown (char *msg)
{
    if (msg && DOSBase)
        _aio_puts(msg);

    // _aio_puts ("shutting down modules...\n");

    if (awindow_init_done)
        _awindow_shutdown();
    if (aio_init_done)
        _aio_shutdown();
    if (autil_init_done)
        _autil_shutdown();

    if (GfxBase) 
        CloseLibrary( (struct Library *)GfxBase);
    if (IntuitionBase) 
        CloseLibrary( (struct Library *)IntuitionBase);

    // _aio_puts ("closing dos.library, exiting...\n");

    if (DOSBase) 
        CloseLibrary( (struct Library *)DOSBase);

    _autil_exit();
}

void _aqb_main(void);

void _cstartup (void)
{
    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 0)))
        _cshutdown("*** error: failed to open dos.library!\n");

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR) "intuition.library", 0)))
        _cshutdown("*** error: failed to open intuition.library!\n");

    if (!(GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0)))
        _cshutdown("*** error: failed to open graphics.library!\n");

    _autil_init();
    autil_init_done = TRUE;

    _aio_init();
    aio_init_done = TRUE;

    _awindow_init();
    awindow_init_done = TRUE;

    _aqb_main();

    _cshutdown(NULL);
}
