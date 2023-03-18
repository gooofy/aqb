/*
 * C part of AQB startup and exit
 *
 * opens libraries, initializes other modules
 * handles clean shutdown on exit
 */

#include "minrt.h"

//#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <exec/execbase.h>
#include <exec/memory.h>

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

//static BOOL autil_init_done = FALSE;
//static BOOL aio_init_done   = FALSE;


ULONG  *_g_stack = NULL;
USHORT   ERR     = 0;

#define MAX_EXIT_HANDLERS 16
static void (*exit_handlers[MAX_EXIT_HANDLERS])(void);
static int num_exit_handlers = 0;

int atexit(void (*cb)(void))
{
    if (num_exit_handlers>=MAX_EXIT_HANDLERS)
        return -1;

    exit_handlers[num_exit_handlers] = cb;
    num_exit_handlers++;

    return 0;
}

// gets called by startup.s' exit()
void _cexit(void)
{
#ifdef ENABLE_DEBUG
    DPRINTF(")cexit...\n");
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

    if (DOSBase)
    {
#ifdef ENABLE_DEBUG
        DPRINTF("_c_atexit: closing dos library\n");
        //Delay(50);
#endif
        CloseLibrary( (struct Library *)DOSBase);
    }

    if (_g_stack)
        FreeVec (_g_stack);
}

void _exit_msg (LONG return_code, UBYTE *msg)
{
    if (msg && DOSBase)
        printf(msg);

    exit(return_code);
}

void _cstartup (void)
{
    SysBase = (*((struct ExecBase **) 4));

    if (!(DOSBase = (struct DOSBase *)OpenLibrary((CONST_STRPTR) "dos.library", 37)))
        _exit_msg(20, (UBYTE *) "*** error: failed to open dos.library!\n");

    _debugger_init();

    if (!(MathBase = (struct MathBase *)OpenLibrary((CONST_STRPTR) "mathffp.library", 37)))
        _exit_msg(20, (UBYTE *) "*** error: failed to open mathffp.library!\n");

    if (!(MathTransBase = (struct MathTransBase *)OpenLibrary((CONST_STRPTR) "mathtrans.library", 37)))
        _exit_msg(20, (UBYTE *) "*** error: failed to open mathtrans.library!\n");

    if (!(UtilityBase = (struct UtilityBase *)OpenLibrary((CONST_STRPTR) "utility.library", 37)))
        _exit_msg(20, (UBYTE *) "*** error: failed to open utility.library!\n");

    DPRINTF ("_cstartup: _aqb_stack_size=%d\n", _aqb_stack_size);
    if (_aqb_stack_size)
    {
        _g_stack = AllocVec (_aqb_stack_size, MEMF_PUBLIC);
        if (!_g_stack)
            _exit_msg(20, (UBYTE *) "*** error: failed to allocate stack!\n");

        DPRINTF ("_cstartup: allocated custom stack: 0x%08lx\n", _g_stack);
    }

    _debugger_init();
}

