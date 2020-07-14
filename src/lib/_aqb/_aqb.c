#include "_aqb.h"

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/intuition_protos.h>
#include <inline/intuition.h>

struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase       *GfxBase       = NULL;

void _aqb_shutdown(void)
{
    _awindow_shutdown();

    if (GfxBase)
        CloseLibrary( (struct Library *)GfxBase);
    if (IntuitionBase)
        CloseLibrary( (struct Library *)IntuitionBase);
}

void __aqb_init(void)
{
    // module initialization - called from __aqb_main

    _aqb_on_exit_call(_aqb_shutdown);

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR) "intuition.library", 0)))
        _cshutdown(20, "*** error: failed to open intuition.library!\n");

    if (!(GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0)))
        _cshutdown(20, "*** error: failed to open graphics.library!\n");

    _awindow_init();
}

