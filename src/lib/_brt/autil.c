
#include "_brt.h"

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/mathffp_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>
#include <inline/mathffp.h>

USHORT g_errcode;

// not using Intuition's AllocRemember here because we want minimal dependencies for the AQB core module

typedef struct AQB_memrec_ *AQB_memrec;

struct AQB_memrec_
{
    AQB_memrec next;
    ULONG      size;
    APTR      *mem;
};

static AQB_memrec g_mem = NULL;

APTR ALLOCATE_(ULONG size, ULONG flags)
{
    AQB_memrec mem_prev = g_mem;

    //_debug_puts("ALLOCATE size=");
    //_debug_puts2(size);
    //_debug_puts(", flags=");
    //_debug_puts2(flags);
    //_debug_puts("\n");

    g_mem = (AQB_memrec) AllocMem (sizeof(*g_mem), 0);
    if (!g_mem)
    {
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->mem = (APTR) AllocMem (size, flags);
    if (!g_mem->mem)
    {
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->size = size;
    g_mem->next = mem_prev;

    return g_mem->mem;
}

void DEALLOCATE (APTR ptr, ULONG size)
{
    // FIXME: implement.
}

static FLOAT f50, f60;

void _autil_init(void)
{
	f60 = SPFlt(60);
    f50 = SPFlt(50);
}

void _autil_shutdown(void)
{
    while (g_mem)
    {
        AQB_memrec mem_next = g_mem->next;
        FreeMem(g_mem->mem, g_mem->size);
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_next;
    }
}

void _aqb_assert (BOOL b, const char *msg)
{
    if (b)
        return;

    _debug_puts(msg);
    _debug_puts("\n");

    _autil_exit(20);
}

static void (*error_handler)(void) = NULL;
static BOOL do_resume = FALSE;

SHORT ERR=0;

void ON_ERROR_CALL(void (*cb)(void))
{
    error_handler = cb;
}

void ERROR (SHORT errcode)
{
    do_resume = FALSE;
    ERR = errcode;

    if (error_handler)
    {
        error_handler();
    }
    else
    {
        _debug_puts("*** unhandled runtime error code: "); _debug_puts2(errcode);
        _debug_puts("\n");
    }

    if (!do_resume)
        _autil_exit(errcode);
    else
        ERR=0;
}

void RESUME_NEXT(void)
{
    do_resume = TRUE;
}

FLOAT TIMER_ (void)
{
	FLOAT res;

	struct DateStamp datetime;

	DateStamp(&datetime);

	res = SPFlt(datetime.ds_Minute);
	res = SPAdd(SPMul(res, f60), SPDiv(f50, SPFlt(datetime.ds_Tick)));

	return res;
}

void SYSTEM(void)
{
    _autil_exit(0);
}

ULONG FRE_(int x)
{

    switch (x)
    {
        case -2:        // stack
        {
            struct Process *Process;
            struct CommandLineInterface *CLI;
            ULONG stack;

            Process = (struct Process *) FindTask (0L);
            if ( (CLI = (struct CommandLineInterface *) (Process -> pr_CLI << 2)) )
            {
                stack = CLI -> cli_DefaultStack << 2;
            }
            else
            {
                stack = Process -> pr_StackSize;
            }
            return stack;
        }
        case -1:        // chip + fast
            return AvailMem(MEMF_CHIP) + AvailMem(MEMF_FAST);
        case 0:         // chip
            return AvailMem(MEMF_CHIP);
        case 1:         // fast
            return AvailMem(MEMF_FAST);
        case 2:         // largest chip
            return AvailMem(MEMF_CHIP|MEMF_LARGEST);
        case 3:         // largest fast
            return AvailMem(MEMF_FAST|MEMF_LARGEST);
        default:
            return 0;
    }
    return 0;
}

void POKE (ULONG adr, UBYTE  b)
{
    UBYTE *p = (UBYTE*)adr;
    *p = b;
}
void POKEW(ULONG adr, USHORT w)
{
    USHORT *p = (USHORT*)adr;
    *p = w;
}
void POKEL(ULONG adr, ULONG  l)
{
    ULONG *p = (ULONG*)adr;
    *p = l;
}

UBYTE PEEK_ (ULONG adr)
{
    UBYTE *p = (UBYTE*)adr;
    return *p;
}

USHORT PEEKW_(ULONG adr)
{
    USHORT *p = (USHORT*)adr;
    return *p;
}

ULONG  PEEKL_(ULONG adr)
{
    ULONG *p = (ULONG*)adr;
    return *p;
}


