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
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;

static BOOL autil_init_done   = FALSE;
static BOOL aio_init_done     = FALSE;

#define MAX_EXIT_HANDLERS 16
static void (*exit_handlers[MAX_EXIT_HANDLERS])(void);
static int num_exit_handlers = 0;

void _aqb_on_exit_call(void (*cb)(void))
{
    if (num_exit_handlers>=MAX_EXIT_HANDLERS)
        return;

    exit_handlers[num_exit_handlers] = cb;
    num_exit_handlers++;
}

// gets called by _autil_exit
void _c_atexit(void)
{
#ifdef ENABLE_DEBUG
    if (DOSBase)
        _aio_puts("_c_atexit...\n");
#endif

    for (int i = num_exit_handlers-1; i>=0; i--)
    {
#ifdef ENABLE_DEBUG
        if (DOSBase)
            _aio_puts("calling user exit handler...\n");
#endif
        exit_handlers[i]();
    }

    if (aio_init_done)
        _aio_shutdown();
    if (autil_init_done)
        _autil_shutdown();

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

    _autil_init();
    autil_init_done = TRUE;

    _astr_init();

    _amath_init();

    _aio_init();
    aio_init_done = TRUE;

    _aqb_main();

    _autil_exit(0);
}

void __aqb_init(void)
{
    // module initialization - called from __aqb_main
}

