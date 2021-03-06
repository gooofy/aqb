#include "_aqb.h"

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/intuition_protos.h>
#include <inline/intuition.h>

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase       *GfxBase       = NULL;

static BOOL aio_init_done      = FALSE;
static BOOL awindow_init_done  = FALSE;
static BOOL atimer_init_done   = FALSE;

void _aqb_shutdown(void)
{
    if (atimer_init_done)
        _atimer_shutdown();
    if (awindow_init_done)
        _awindow_shutdown();
    if (aio_init_done)
        _aio_shutdown();

    if (GfxBase)
        CloseLibrary( (struct Library *)GfxBase);
    if (IntuitionBase)
        CloseLibrary( (struct Library *)IntuitionBase);
}

void __aqb_init(void)
{
    // module initialization - called from __aqb_main

    ON_EXIT_CALL(_aqb_shutdown);

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR) "intuition.library", 0)))
        _cshutdown(20, (UBYTE *)"*** error: failed to open intuition.library!\n");

    if (!(GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0)))
        _cshutdown(20, (UBYTE *)"*** error: failed to open graphics.library!\n");

    _awindow_init();
    awindow_init_done = TRUE;

    _aio_init();
    aio_init_done = TRUE;

    _atimer_init();
    atimer_init_done = TRUE;
}

