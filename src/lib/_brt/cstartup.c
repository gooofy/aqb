/*
 * C part of AQB startup
 *
 * opens libraries, initializes other modules,
 * calls __aqbmain
 * and shuts down everything once __aqbmain returns
 */

#include "_brt.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/execbase.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

// #define ENABLE_DEBUG
#define MAXBUF 40

struct ExecBase      *SysBase       = NULL;
struct DOSBase       *DOSBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;

static BOOL autil_init_done = FALSE;

static BPTR _debug_stdout = 0;

void _debug_puts(const char *s)
{
    if (_debug_stdout)
        Write(_debug_stdout, (CONST APTR) s, len_(s));
}

void _debug_puts2(SHORT s)
{
    char buf[MAXBUF];
    _astr_itoa(s, buf, 10);
    _debug_puts(buf);
}

void _debug_putu4(ULONG l)
{
    char buf[MAXBUF];
    _astr_itoa(l, buf, 10);
    _debug_puts(buf);
}

void _debug_putnl(void)
{
    if (_debug_stdout)
        Write(_debug_stdout, "\n", 1);
}

#define MAX_EXIT_HANDLERS 16
static void (*exit_handlers[MAX_EXIT_HANDLERS])(void);
static int num_exit_handlers = 0;

void ON_EXIT_CALL(void (*cb)(void))
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
        _debug_puts("_c_atexit...\n");
#endif

    for (int i = num_exit_handlers-1; i>=0; i--)
    {
#ifdef ENABLE_DEBUG
        if (DOSBase)
            _debug_puts("calling user exit handler...\n");
#endif
        exit_handlers[i]();
    }

    if (autil_init_done)
        _autil_shutdown();

    if (MathTransBase)
        CloseLibrary( (struct Library *)MathTransBase);
    if (MathBase)
        CloseLibrary( (struct Library *)MathBase);

#ifdef ENABLE_DEBUG
    if (DOSBase)
        _debug_puts("_c_atexit... finishing.\n");
#endif

    if (DOSBase)
        CloseLibrary( (struct Library *)DOSBase);
}

void _cshutdown (LONG return_code, char *msg)
{
    if (msg && DOSBase)
        _debug_puts(msg);

    _autil_exit(return_code);
}

void _aqb_main(void);

void _cstartup (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 0)))
        _cshutdown(20, "*** error: failed to open dos.library!\n");

    _debug_stdout = Output();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 0)))
        _cshutdown(20, "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 0)))
        _cshutdown(20, "*** error: failed to open mathtrans.library!\n");

    _autil_init();
    autil_init_done = TRUE;

    _astr_init();

    _amath_init();

    _aqb_main();

    _autil_exit(0);
}

void __brt_init(void)
{
    // module initialization - called from __aqb_main
}

