/*
 * C part of AQB startup and exit
 *
 * opens libraries, initializes other modules
 * handles clean shutdown on exit
 */

//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>

#include <exec/execbase.h>
#include <exec/memory.h>

#include <devices/inputevent.h>
#include <devices/input.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/utility_protos.h>
#include <inline/utility.h>

//#define ENABLE_DEBUG

struct ExecBase      *SysBase       = NULL;
struct DOSBase       *DOSBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;
struct UtilityBase   *UtilityBase   = NULL;

static BOOL autil_init_done = FALSE;
static BOOL aio_init_done   = FALSE;

extern struct DebugMsg  *__StartupMsg;
USHORT                   _startup_mode           = 0;
static BPTR              _debug_stdout           = 0;
static struct DebugMsg   _dbgOutputMsg;
static struct MsgPort   *g_dbgPort               = NULL;

ULONG *_g_stack = NULL;

asm(
"   .text\n"
"   .align 2\n"
"   .globl  __debug_break\n"
"   __debug_break:\n"
"		link    a5, #0;\n"
"		trap    #1;\n"
"		unlk    a5;\n"
"		rts;\n"
);

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
    DPRINTF("_c_atexit...\n");
    //Delay(50);
#endif

    for (int i = num_exit_handlers-1; i>=0; i--)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: calling exit handler #%d/%d...\n", i+1, num_exit_handlers);
        //Delay(50);
#endif
        exit_handlers[i]();
    }

    if (g_InputHandlerInstalled)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: remove input handler\n");
        //Delay(50);
#endif
        g_inputReqBlk->io_Data    = (APTR)g_inputHandler;
        g_inputReqBlk->io_Command = IND_REMHANDLER;

        DoIO((struct IORequest *)g_inputReqBlk);
    }

    if (g_inputDeviceOpen)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: close input.device\n");
        //Delay(50);
#endif
        CloseDevice((struct IORequest *)g_inputReqBlk);
    }

    if (g_inputReqBlk)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: delete input io req\n");
        //Delay(50);
#endif
        _autil_delete_ext_io((struct IORequest *)g_inputReqBlk);
    }

    if (g_inputHandler)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: free input handler\n");
        //Delay(50);
#endif
        FreeMem(g_inputHandler, sizeof(struct Interrupt));
    }

    if (g_inputPort)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: delete input port\n");
        //Delay(50);
#endif
        _autil_delete_port(g_inputPort);
    }

    if (aio_init_done)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: _aio_shutdown\n");
        //Delay(50);
#endif
        _aio_shutdown();
    }

    if (autil_init_done)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: _autil_shutdown\n");
        //Delay(50);
#endif
        _autil_shutdown();
    }

    if (UtilityBase)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: close utility.library\n");
        //Delay(50);
#endif
        CloseLibrary((struct Library *)UtilityBase);
    }
    if (MathTransBase)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: close mathtrans.library\n");
        //Delay(50);
#endif
        CloseLibrary( (struct Library *)MathTransBase);
    }
    if (MathBase)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: close mathbase.library\n");
        //Delay(50);
#endif
        CloseLibrary( (struct Library *)MathBase);
    }

#ifdef ENABLE_DEBUG
    DPRINTF("_c_atexit: finishing.\n");
    //Delay(50);
#endif

    if (g_dbgPort)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: delete dbg port\n");
        //Delay(50);
#endif
        _autil_delete_port(g_dbgPort);
        g_dbgPort = NULL;
    }

    if (DOSBase)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: closing dos library\n");
        //Delay(50);
#endif
        _debug_stdout = 0;
        CloseLibrary( (struct Library *)DOSBase);
    }

    if (_g_stack)
        FreeVec (_g_stack);
}

void _cshutdown (LONG return_code, UBYTE *msg)
{
    if (msg && DOSBase)
        _debug_puts(msg);

    _autil_exit(return_code);
}

void _cstartup (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open dos.library!\n");

    _debug_stdout = Output();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open mathtrans.library!\n");

    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary((CONST_STRPTR) "utility.library", 37)))
        _cshutdown(20, (UBYTE *) "*** error: failed to open utility.library!\n");

    DPRINTF ("_cstartup: _aqb_stack_size=%d\n", _aqb_stack_size);
    if (_aqb_stack_size)
    {
        _g_stack = AllocVec (_aqb_stack_size, MEMF_PUBLIC);
        if (!_g_stack)
            _cshutdown(20, (UBYTE *) "*** error: failed to allocate stack!\n");

        DPRINTF ("_cstartup: allocated custom stack: 0x%08lx\n", _g_stack);
    }

    // FIXME

    //_autil_init();
    //autil_init_done = TRUE;

    //_debugger_init();

    //_astr_init();

    //_amath_init();

    //_aio_init();
    //aio_init_done = TRUE;
}

