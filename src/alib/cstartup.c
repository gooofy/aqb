/*
 * C part of AQB startup
 *
 * opens libraries, initializes other modules,
 * calls __aqbmain
 * and shuts down everything once __aqbmain returns
 */

#include "aio.h"
#include "astr.h"
#include "autil.h"
#include "awindow.h"
#include "amath.h"

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

// #define ENABLE_DEBUG

struct DOSBase       *DOSBase       = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct GfxBase       *GfxBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;

static BOOL awindow_init_done = FALSE;
static BOOL autil_init_done   = FALSE;
static BOOL aio_init_done     = FALSE;

// gets called by _autil_exit
void _c_atexit(void)
{
#ifdef ENABLE_DEBUG
    if (DOSBase)
        _aio_puts("_c_atexit...\n");
#endif

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
    if (MathTransBase)
        CloseLibrary( (struct Library *)MathTransBase);
    if (MathBase)
        CloseLibrary( (struct Library *)MathBase);

#ifdef ENABLE_DEBUG
    if (DOSBase)
        _aio_puts("_c_atexit... finishing.\n");
#endif

    if (DOSBase)
        CloseLibrary( (struct Library *)DOSBase);
}

static void _cshutdown (LONG return_code, char *msg)
{
    if (msg && DOSBase)
        _aio_puts(msg);

    _autil_exit(return_code);
}

void _aqb_main(void);

void _cstartup (void)
{
    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 0)))
        _cshutdown(20, "*** error: failed to open dos.library!\n");

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 0)))
        _cshutdown(20, "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 0)))
        _cshutdown(20, "*** error: failed to open mathtrans.library!\n");

    if (!(IntuitionBase = (struct IntuitionBase *)OpenLibrary((CONST_STRPTR) "intuition.library", 0)))
        _cshutdown(20, "*** error: failed to open intuition.library!\n");

    if (!(GfxBase = (struct GfxBase *)OpenLibrary((CONST_STRPTR) "graphics.library", 0)))
        _cshutdown(20, "*** error: failed to open graphics.library!\n");

    _autil_init();
    autil_init_done = TRUE;

    _astr_init();

    _amath_init();

    _aio_init();
    aio_init_done = TRUE;

    _awindow_init();
    awindow_init_done = TRUE;

    _aqb_main();

    _autil_exit(0);
}
