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

#include <devices/inputevent.h>
#include <devices/input.h>

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

struct Task          *_autil_task             = NULL;
struct IOStdReq      *g_inputReqBlk           = NULL;
struct MsgPort       *g_inputPort             = NULL;
struct Interrupt     *g_inputHandler          = NULL;
static BOOL           g_inputDeviceOpen       = FALSE;
static BOOL           g_InputHandlerInstalled = FALSE;


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

USHORT _break_status = 0;

void __handle_break(void)
{
    if (_break_status)  // FIXME: debug breaks
        _autil_exit(1);
}

static APTR ___inputHandler ( register struct InputEvent *oldEventChain __asm("a0"),
                            register APTR               data          __asm("a1"))
{
    //LOG_printf (LOG_INFO, "___inputHandler called: oldEventChain=0x%08lx data=%ld\n", (ULONG) oldEventChain, (ULONG) data);

    struct InputEvent *e = oldEventChain;
    while (e)
    {
        if ( (e->ie_Class == IECLASS_RAWKEY) && ((e->ie_Code & 0x7f) == 0x33) && (e->ie_Qualifier & IEQUALIFIER_CONTROL) )
        {
            struct Task *maintask = data;
            Signal (maintask, SIGBREAKF_CTRL_C);
        }

        e = e->ie_NextEvent;
    }

	return oldEventChain;
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

    if (g_InputHandlerInstalled)
    {
        g_inputReqBlk->io_Data    = (APTR)g_inputHandler;
        g_inputReqBlk->io_Command = IND_REMHANDLER;

        DoIO((struct IORequest *)g_inputReqBlk);
    }

    if (g_inputDeviceOpen)
        CloseDevice((struct IORequest *)g_inputReqBlk);

    if (g_inputReqBlk)
        _autil_delete_ext_io((struct IORequest *)g_inputReqBlk);

    if (g_inputHandler)
        FreeMem(g_inputHandler, sizeof(struct Interrupt));

    if (g_inputPort)
        _autil_delete_port(g_inputPort);

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

void ___breakHandler (register ULONG signals __asm("d0"), register APTR exceptData __asm("a1"))
{
    _break_status = BREAK_CTRL_C;
#if 0
    // dos call pending ?

    Forbid();
    ULONG sigWait  = g_task->tc_SigWait;
    ULONG sigRecvd = g_task->tc_SigRecvd;
    BOOL inDos = (sigWait ^ sigRecvd) & SIGF_DOS;

    if (inDos)
    {
        Permit();
        return;
    }

    //Permit();

    //_debug_puts ((STRPTR)"\n\n*** ___breakHandler called g_task->tc_SigWait=");
    //_debug_putu4 (sigWait);
    //_debug_puts ((STRPTR)" g_task->tc_SigRecvd=");
    //_debug_putu4 (sigRecvd);
    //_debug_puts ((STRPTR)" inDos=");
    //_debug_puts2 (inDos);
    //_debug_puts ((STRPTR)"\n\n");

    //_breakCode = BREAK_CTRL_C;
    _autil_exit(1);
#endif
}

static char *g_inputHandlerName = "AQB CTRL-C input event handler";

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

    _autil_task = FindTask(NULL);

    Forbid();
    _autil_task->tc_ExceptData = NULL;
    _autil_task->tc_ExceptCode = ___breakHandler;
    SetSignal (0, SIGBREAKF_CTRL_C);
    SetExcept (SIGBREAKF_CTRL_C, SIGBREAKF_CTRL_C);
    Permit();

    /* install CTRL+C input event handler */

	if ( !(g_inputPort=_autil_create_port(NULL, 0)) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C handler input port!\n");

    if ( !(g_inputHandler=AllocMem(sizeof(struct Interrupt), MEMF_PUBLIC|MEMF_CLEAR)) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C handler memory!\n");

    if ( !(g_inputReqBlk=(struct IOStdReq *)_autil_create_ext_io(g_inputPort, sizeof(struct IOStdReq))) )
        _cshutdown(20, (UBYTE *) "*** error: failed to allocate CTRL-C ext io!\n");

    if (OpenDevice ((STRPTR)"input.device", /*unitNumber=*/0, (struct IORequest *)g_inputReqBlk, /*flags=*/0))
        _cshutdown(20, (UBYTE *) "*** error: failed to open input.device!\n");
    g_inputDeviceOpen = TRUE;

    g_inputHandler->is_Code         = (APTR) ___inputHandler;
    g_inputHandler->is_Data         = (APTR) _autil_task;
    g_inputHandler->is_Node.ln_Pri  = 100;
    g_inputHandler->is_Node.ln_Name = g_inputHandlerName;

    g_inputReqBlk->io_Data    = (APTR)g_inputHandler;
    g_inputReqBlk->io_Command = IND_ADDHANDLER;

    DoIO((struct IORequest *)g_inputReqBlk);
    g_InputHandlerInstalled = TRUE;

    _astr_init();

    _amath_init();
}

void __brt_init(void)
{
    // module initialization - called from __aqb_main
}

