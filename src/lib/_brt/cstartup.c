/*
 * C part of AQB startup and exit
 *
 * opens libraries, initializes other modules
 * handles clean shutdown on exit
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

void _debug_puts(const UBYTE *s)
{
    if (_debug_stdout)
        Write(_debug_stdout, (CONST APTR) s, LEN_(s));
}

void _debug_puts2(SHORT s)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(s, buf, 10);
    _debug_puts(buf);
}

void _debug_putu4(ULONG l)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(l, buf, 10);
    _debug_puts(buf);
}

void _debug_putf(FLOAT f)
{
    UBYTE buf[MAXBUF];
    _astr_ftoa(f, buf);
    _debug_puts(buf);
}

void _debug_putnl(void)
{
    if (_debug_stdout)
        Write(_debug_stdout, "\n", 1);
}

void _debug_cls(void)
{
    if (_debug_stdout)
        Write(_debug_stdout, "\f", 1);
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

void _cshutdown (LONG return_code, UBYTE *msg)
{
    if (msg && DOSBase)
        _debug_puts(msg);

    _autil_exit(return_code);
}

static struct Task *g_task = NULL;

//USHORT _breakCode = 0;

static void _breakHandler (register ULONG signals __asm("d0"), register APTR exceptData __asm("a1"))
{
    // dos call pending ?

    Forbid();
    BOOL inDos = (g_task->tc_SigWait ^ g_task->tc_SigRecvd) & SIGF_DOS;
    Permit();

    if (inDos)
        return;

    //_breakCode = BREAK_CTRL_C;
    _debug_puts ((STRPTR)"\n\n*** _breakHandler called.\n\n");
    _autil_exit(1);
}

void _cstartup (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 0)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open dos.library!\n");

    _debug_stdout = Output();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 0)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 0)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathtrans.library!\n");

    _autil_init();
    autil_init_done = TRUE;

    /* set up break signal exception + handler */

    struct Task *g_task = FindTask(NULL);

    Forbid();
    g_task->tc_ExceptData = NULL;
    g_task->tc_ExceptCode = _breakHandler;
    SetSignal (0, SIGBREAKF_CTRL_C);
    SetExcept (SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
    Permit();

    _astr_init();

    _amath_init();
}

void __brt_init(void)
{
    // module initialization - called from __aqb_main
}

