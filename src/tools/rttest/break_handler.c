#include "minrt.h"

USHORT _break_status = 0;

struct Task             *_autil_task             = NULL;
static struct IOStdReq  *g_inputReqBlk           = NULL;
static struct MsgPort   *g_inputPort             = NULL;
static struct Interrupt *g_inputHandler          = NULL;
static BOOL              g_inputDeviceOpen       = FALSE;
static BOOL              g_InputHandlerInstalled = FALSE;

static char *g_inputHandlerName = "AQB CTRL-C input event handler";

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

static void ___breakHandler (register ULONG signals __asm("d0"), register APTR exceptData __asm("a1"))
{
    _break_status = BREAK_CTRL_C;
}

void _break_handler_init(void)
{
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
}

