#include "minrt.h"

#include <string.h>
#include <stdlib.h>

#include <clib/alib_protos.h>

#include <dos/dos.h>
#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <exec/ports.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#define DEBUG_SIG          0xDECA11ED
#define DEBUG_CMD_START    23
#define DEBUG_CMD_PUTC     24
#define DEBUG_CMD_PUTS     25

/* debugger sends this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
    struct MsgPort *port;
    ULONG           debug_sig;                  // 24
    UWORD           debug_cmd;                  // 28
    ULONG           debug_exitFn;               // 30
    union
    {
        ULONG   err;    // START return msg     // 34
        char    c;      // putc                 // 34
        char   *str;    // puts                 // 34
    }u;
};

extern struct DebugMsg  *__StartupMsg;
USHORT                   _startup_mode           = 0;
static BPTR              _debug_stdout           = 0;
static struct MsgPort   *g_dbgPort               = NULL;
static struct DebugMsg   _dbgOutputMsg;

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

static void _debug_putc(const char c)
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

//static void _debug_puts(const UBYTE *s)
//{
//    if ((_startup_mode == STARTUP_DEBUG) && g_dbgPort)
//	{
//        _dbgOutputMsg.msg.mn_Node.ln_Succ = NULL;
//        _dbgOutputMsg.msg.mn_Node.ln_Pred = NULL;
//        _dbgOutputMsg.msg.mn_Node.ln_Pri  = 0;
//        _dbgOutputMsg.msg.mn_Node.ln_Name = NULL;
//		_dbgOutputMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
//		_dbgOutputMsg.msg.mn_Length       = sizeof(struct DebugMsg);
//		_dbgOutputMsg.msg.mn_ReplyPort    = g_dbgPort;
//		_dbgOutputMsg.debug_sig           = DEBUG_SIG;
//		_dbgOutputMsg.debug_cmd           = DEBUG_CMD_PUTS;
//		_dbgOutputMsg.u.str               = (char *) s;
//
//        // Write(_debug_stdout, (STRPTR) "_debug_puts:2PutMsg\n", 20);
//		PutMsg (__StartupMsg->msg.mn_ReplyPort, &_dbgOutputMsg.msg);
//		WaitPort(g_dbgPort);
//        GetMsg(g_dbgPort); // discard reply
//    }
//    else
//    {
//		if (_debug_stdout)
//			Write(_debug_stdout, (CONST APTR) s, strlen(s));
//	}
//}

int printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vcbprintf(format, args, _debug_putc);
    va_end(args);

    return 0;
}

static void _debugger_shutdown(void)
{
    if (g_dbgPort)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: delete dbg port\n");
        //Delay(50);
#endif
        DeletePort(g_dbgPort);
        g_dbgPort = NULL;
    }
    _debug_stdout = 0;
}

void _debugger_init(void)
{
    atexit (_debugger_shutdown);

    _debug_stdout = Output();

    if (!__StartupMsg)
    {
        _startup_mode = STARTUP_CLI;
    }
    else
    {
		if (__StartupMsg->debug_sig == DEBUG_SIG)
		{
			_startup_mode = STARTUP_DEBUG;
			if ( !(g_dbgPort=CreatePort(NULL, 0)) )
				_exit_msg(20, (UBYTE *) "*** error: failed to allocate debug port!\n");
		}
		else
		{
			_startup_mode = STARTUP_WBENCH;
		}
    }
}
