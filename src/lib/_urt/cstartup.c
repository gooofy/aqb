/*
 * C part of ACS startup and exit
 *
 * opens libraries, initializes other modules
 * handles clean shutdown on exit
 */

#include "_urt.h"

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

#include <clib/utility_protos.h>
#include <inline/utility.h>

//#define ENABLE_DEBUG
#define MAXBUF 40

struct ExecBase      *SysBase       = NULL;
struct DOSBase       *DOSBase       = NULL;
struct MathBase      *MathBase      = NULL;
struct MathTransBase *MathTransBase = NULL;
struct UtilityBase   *UtilityBase   = NULL;

static BOOL autil_init_done   = FALSE;
static BOOL console_init_done = FALSE;
static BOOL gc_init_done      = FALSE;

struct Task             *_autil_task             = NULL;
static struct IOStdReq  *g_inputReqBlk           = NULL;
static struct MsgPort   *g_inputPort             = NULL;
static struct Interrupt *g_inputHandler          = NULL;
static BOOL              g_inputDeviceOpen       = FALSE;
static BOOL              g_InputHandlerInstalled = FALSE;

extern struct DebugMsg  *__StartupMsg;
USHORT                   _startup_mode           = 0;
static BPTR              _debug_stdout           = 0;
static struct DebugMsg   _dbgOutputMsg;
static struct MsgPort   *g_dbgPort               = NULL;

static char *g_inputHandlerName = "ACS CTRL-C input event handler";

ULONG *_g_stack = NULL;

void __debug_put_int8(int8_t c)
{
    if ((_startup_mode == STARTUP_DEBUG) && g_dbgPort)
	{
        _dbgOutputMsg.msg.mn_Node.ln_Succ = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pred = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pri  = 0;
        _dbgOutputMsg.msg.mn_Node.ln_Name = NULL;
		_dbgOutputMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
		_dbgOutputMsg.msg.mn_Length       = sizeof(struct DebugMsg);
		_dbgOutputMsg.msg.mn_ReplyPort    = g_dbgPort;
		_dbgOutputMsg.debug_sig           = DEBUG_SIG;
		_dbgOutputMsg.debug_cmd           = DEBUG_CMD_PUTC;
		_dbgOutputMsg.u.c                 = c;

		PutMsg (__StartupMsg->msg.mn_ReplyPort, &_dbgOutputMsg.msg);
		WaitPort(g_dbgPort);
        GetMsg(g_dbgPort); // discard reply
	}
    else
    {
        if (_debug_stdout)
            Write(_debug_stdout, (CONST APTR) &c, 1);
    }
}

void __debug_puts(CONST_STRPTR s)
{
    if ((_startup_mode == STARTUP_DEBUG) && g_dbgPort)
	{
        _dbgOutputMsg.msg.mn_Node.ln_Succ = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pred = NULL;
        _dbgOutputMsg.msg.mn_Node.ln_Pri  = 0;
        _dbgOutputMsg.msg.mn_Node.ln_Name = NULL;
		_dbgOutputMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
		_dbgOutputMsg.msg.mn_Length       = sizeof(struct DebugMsg);
		_dbgOutputMsg.msg.mn_ReplyPort    = g_dbgPort;
		_dbgOutputMsg.debug_sig           = DEBUG_SIG;
		_dbgOutputMsg.debug_cmd           = DEBUG_CMD_PUTS;
		_dbgOutputMsg.u.str               = (char *) s;

        // Write(_debug_stdout, (STRPTR) "_debug_puts:2PutMsg\n", 20);
		PutMsg (__StartupMsg->msg.mn_ReplyPort, &_dbgOutputMsg.msg);
		WaitPort(g_dbgPort);
        GetMsg(g_dbgPort); // discard reply
    }
    else
    {
		if (_debug_stdout)
			Write(_debug_stdout, (CONST APTR) s, _astr_len((STRPTR)s));
	}
}

//void _DEBUG_PUTS(const CString *s)
//{
//    __debug_puts (s->_str);
//}
//
//void _DEBUG_PUTS1(BYTE s)
//{
//    UBYTE buf[MAXBUF];
//    _astr_itoa(s, buf, 10);
//    __debug_puts(buf);
//}

void __debug_put_int16(SHORT s)
{
    UBYTE buf[MAXBUF];
    _astr_itoa(s, buf, 10);
    __debug_puts(buf);
}

//void _DEBUG_PUTS4(LONG l)
//{
//    UBYTE buf[MAXBUF];
//    _astr_itoa(l, buf, 10);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTU1(UBYTE num)
//{
//    UBYTE buf[MAXBUF];
//    _astr_utoa(num, buf, 10);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTU2(UWORD num)
//{
//    UBYTE buf[MAXBUF];
//    _astr_utoa(num, buf, 10);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTU4(ULONG l)
//{
//    UBYTE buf[MAXBUF];
//    _astr_utoa(l, buf, 10);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTHEX(ULONG l)
//{
//    UBYTE buf[MAXBUF];
//    _astr_utoa(l, buf, 16);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTF(FLOAT f)
//{
//    UBYTE buf[MAXBUF];
//    _astr_ftoa(f, buf);
//    __debug_puts(buf);
//}
//
//void _DEBUG_PUTBOOL(BOOL b)
//{
//    __debug_puts(b ? (STRPTR)"TRUE" : (STRPTR)"FALSE");
//}
//
//void _DEBUG_PUTTAB(void)
//{
//    _DEBUG_PUTC('\t');
//}
//
//void _DEBUG_PUTNL(void)
//{
//    _DEBUG_PUTC('\n');
//}
//
//void _DEBUG_CLS(void)
//{
//    _DEBUG_PUTC('\f');
//}

asm(
"   .text\n"
"   .align 2\n"
"   .globl  __DEBUG_BREAK\n"
"   __DEBUG_BREAK:\n"
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

USHORT _break_status = 0;

asm(
"   .text\n"
"   .align 2\n"
"   .globl  __ctrlc_break\n"
"   __ctrlc_break:\n"
"		link    a5, #0;\n"
"		trap    #0;\n"
"		unlk    a5;\n"
"		rts;\n"
);

void _ctrlc_break(void);

static void (*break_handler)(void) = NULL;

void ON_BREAK_CALL(void (*cb)(void))
{
    break_handler = cb;
}

void __handle_break(void)
{
    DPRINTF ("__handle_break...\n");
    if (_break_status)
    {
        DPRINTF ("__handle_break... _break_status!=0\n");
        _break_status = 0;

        if (break_handler)
        {
            _do_resume = FALSE;
            break_handler();
            if (_do_resume)
                return;
        }

        if (_startup_mode == STARTUP_DEBUG)
        {
            DPRINTF ("__handle_break... -> TRAP\n");
            //asm ("  trap #0;\n");           // break into debugger
            _ctrlc_break();
        }
        else
        {
            DPRINTF ("__handle_break... -> exit\n");
            _autil_exit(1);
        }
    }
}

static APTR ___inputHandler ( register struct InputEvent *oldEventChain __asm("a0"),
                            register APTR               data          __asm("a1"))
{
    //LOG_printf (LOG_INFO, "___inputHandler called: oldEventChain=0x%08lx data=%ld\n", (ULONG) oldEventChain, (ULONG) data);

    struct InputEvent *e = oldEventChain;
    while (e)
    {
        if ( (e->ie_Class == IECLASS_RAWKEY) && ((e->ie_Code & 0x7f) == 0x33) && ((e->ie_Code & IECODE_UP_PREFIX)==0) && (e->ie_Qualifier & IEQUALIFIER_CONTROL) )
        {
            //DPRINTF("___inputHandler: CTRL-C detected, e->ie_Code=0x%08lx\n", e->ie_Code);
            _break_status = BREAK_CTRL_C;
            //struct Task *maintask = data;
            //Signal (maintask, SIGBREAKF_CTRL_C);
        }

        e = e->ie_NextEvent;
    }

	return oldEventChain;
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

    if (console_init_done)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: _aio_shutdown\n");
        //Delay(50);
#endif
        _console_shutdown();
    }

    if (gc_init_done)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: _gc_shutdown\n");
        //Delay(50);
#endif
        _gc_shutdown();
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
        __debug_puts(msg);

    _autil_exit(return_code);
}

void ___breakHandler (register ULONG signals __asm("d0"), register APTR exceptData __asm("a1"))
{
    _break_status = BREAK_CTRL_C;
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

    DPRINTF ("_cstartup: _acs_stack_size=%d\n", _acs_stack_size);
    if (_acs_stack_size)
    {
        _g_stack = AllocVec (_acs_stack_size, MEMF_PUBLIC);
        if (!_g_stack)
            _cshutdown(20, (UBYTE *) "*** error: failed to allocate stack!\n");

        DPRINTF ("_cstartup: allocated custom stack: 0x%08lx\n", _g_stack);
    }

    _autil_init();
    autil_init_done = TRUE;

    // detect startup mode

    if (!__StartupMsg)
    {
        _startup_mode = STARTUP_CLI;
    }
    else
    {
		if (__StartupMsg->debug_sig == DEBUG_SIG)
		{
			_startup_mode = STARTUP_DEBUG;
			if ( !(g_dbgPort=_autil_create_port(NULL, 0)) )
				_cshutdown(20, (UBYTE *) "*** error: failed to allocate debug port!\n");
		}
		else
		{
			_startup_mode = STARTUP_WBENCH;
		}
    }

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

    _gc_init();
    gc_init_done = TRUE;

    _astr_init();

    _amath_init();

    _console_init();
    console_init_done = TRUE;
}


