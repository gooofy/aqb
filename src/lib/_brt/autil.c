
//#define ENABLE_DPRINTF
//#define MEMDEBUG

#include "_brt.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <proto/exec.h>
#include <clib/exec_protos.h>
#include <inline/exec.h>

#include <clib/dos_protos.h>
#include <inline/dos.h>

#include <clib/mathffp_protos.h>
#include <inline/mathffp.h>

#include <clib/utility_protos.h>
#include <inline/utility.h>

extern struct UtilityBase   *UtilityBase;

// not using Intuition's AllocRemember here because we want minimal dependencies for the AQB core module

typedef struct AQB_memrec_ *AQB_memrec;

struct AQB_memrec_
{
    AQB_memrec next;
    ULONG      size;
#ifdef MEMDEBUG
    ULONG      marker1;
#endif
    APTR      *mem;
#ifdef MEMDEBUG
    ULONG      marker2;
#endif
};

#define MARKER1 0xAFFE1234
#define MARKER2 0xCAFEBABE

static AQB_memrec g_mem = NULL;

_autil_sleep_for_cb_t _autil_sleep_for_cb = NULL;
static FLOAT g_fp50, g_fp60;

APTR ALLOCATE_(ULONG size, ULONG flags)
{
    AQB_memrec mem_prev = g_mem;

    g_mem = (AQB_memrec) AllocMem (sizeof(*g_mem), 0);
    if (!g_mem)
    {
        DPRINTF ("ALLOCATE_: OOM1\n");
        g_mem = mem_prev;
        return NULL;
    }

    g_mem->mem = (APTR) AllocMem (size, flags);
    if (!g_mem->mem)
    {
        DPRINTF ("ALLOCATE_: OOM1\n");
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_prev;
        return NULL;
    }

    DPRINTF ("ALLOCATE_: size=%ld, flags=%ld -> 0x%08lx\n", size, flags, g_mem->mem);

    g_mem->size = size;
    g_mem->next = mem_prev;

#ifdef MEMDEBUG
    g_mem->marker1 = MARKER1;
    g_mem->marker2 = MARKER2;
    _MEMSET ((BYTE*)g_mem->mem, 0xEF, size);
#endif

    return g_mem->mem;
}

void DEALLOCATE (APTR ptr)
{
    // FIXME: implement.
}

void _MEMSET (BYTE *dst, BYTE c, ULONG n)
{
    if (n)
	{
        BYTE *d = dst;

        do
            *d++ = c;
        while (--n != 0);
    }
}

void _AQB_ASSERT (BOOL b, const UBYTE *msg)
{
    if (b)
        return;

    _DEBUG_PUTS(msg);
    _DEBUG_PUTS((UBYTE *)"\n");

    if (_startup_mode == STARTUP_DEBUG)
    {
        asm ("  trap #2;\n");           // break into debugger
    }
    else
    {
        _autil_exit(20);
    }
}

static void (*error_handler)(void) = NULL;
BOOL _do_resume = FALSE;

SHORT ERR=0;

void ON_ERROR_CALL(void (*cb)(void))
{
    error_handler = cb;
}

void ERROR (SHORT errcode)
{
    _do_resume = FALSE;
    ERR = errcode;

    if (error_handler)
    {
        error_handler();
    }
    else
    {
        _DEBUG_PUTS((UBYTE*)"*** unhandled runtime error code: "); _DEBUG_PUTS2(errcode);
        _DEBUG_PUTS((UBYTE*)"\n");
    }

    if (!_do_resume)
    {
        if (_startup_mode == STARTUP_DEBUG)
        {
            __StartupMsg->u.err = errcode;
            asm ("  trap #3;\n");           // break into debugger
        }
        else
        {
            _autil_exit(errcode);
        }
    }
    else
    {
        ERR=0;
        _do_resume = FALSE;
    }
}

void RESUME_NEXT(void)
{
    _do_resume = TRUE;
}

FLOAT TIMER_ (void)
{
	FLOAT res;

	struct DateStamp datetime;

	DateStamp(&datetime);

	res = SPFlt(datetime.ds_Minute);
	res = SPAdd(SPMul(res, g_fp60), SPDiv(g_fp50, SPFlt(datetime.ds_Tick)));

	return res;
}

STRPTR DATE_ (void)
{
    struct DateStamp datetime;
    LONG             seconds;
    struct ClockData cd;

	DateStamp(&datetime);

    seconds = datetime.ds_Days*24*60*60 + datetime.ds_Minute*60 + datetime.ds_Tick/TICKS_PER_SECOND;

    DPRINTF ("DATE$: seconds=%ld\n", seconds);

    Amiga2Date (seconds, &cd);

    DPRINTF ("DATE$: %02d-%02d-%04d %02d:%02d:%02d\n", cd.month, cd.mday, cd.year, cd.hour, cd.min, cd.sec);

    char buf[10] = "00-00-0000";

    buf[0] = '0' + cd.month/10;
    buf[1] = '0' + cd.month%10;

    buf[3] = '0' + cd.mday/10;
    buf[4] = '0' + cd.mday%10;

    LONG y = cd.year;
    buf[6] = '0' + y/1000;
    y %= 1000;
    buf[7] = '0' + y/100;
    y %= 100;
    buf[8] = '0' + y/10;
    buf[9] = '0' + y%10;

    return _astr_dup((STRPTR)buf);
}

void SLEEP_FOR (FLOAT s)
{
    if (_autil_sleep_for_cb)
    {
        _autil_sleep_for_cb (s);
        return;
    }
    LONG ticks = SPFix(SPMul(s, g_fp50));
    Delay (ticks);
}

void SYSTEM(void)
{
    _autil_exit(0);
}

ULONG FRE_(SHORT x)
{

    switch (x)
    {
        case -2:        // stack
        {
            APTR  upper, lower;
            ULONG total;

            struct Process *pr = (struct Process*) FindTask (0L);

            if ( (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS) && pr->pr_CLI && !_g_stack )
            {
                upper = (APTR) pr->pr_ReturnAddr + 4;
                total = * ((ULONG *)pr->pr_ReturnAddr);
                lower = upper-total;
            }
            else
            {
                upper = pr->pr_Task.tc_SPUpper;
                lower = pr->pr_Task.tc_SPLower;
                total = upper-lower;
            }

            return total;
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

/* origin: libnix */

struct MsgPort *_autil_create_port(STRPTR name, LONG pri)
{
    APTR SysBase = *(APTR *)4L;
    struct MsgPort *port = NULL;
    UBYTE portsig;

    if ((BYTE)(portsig=AllocSignal(-1)) >= 0)
    {
        if (!(port=AllocMem(sizeof(*port),MEMF_CLEAR|MEMF_PUBLIC)))
        {
            FreeSignal(portsig);
        }
        else
        {
            port->mp_Node.ln_Type = NT_MSGPORT;
            port->mp_Node.ln_Pri  = pri;
            port->mp_Node.ln_Name = (char *)name;
            /* done via AllocMem
            port->mp_Flags        = PA_SIGNAL;
            */
            port->mp_SigBit       = portsig;
            port->mp_SigTask      = FindTask(NULL);
            NEWLIST(&port->mp_MsgList);
            if (port->mp_Node.ln_Name)
                AddPort(port);
        }
    }
    return port;
}

void _autil_delete_port(struct MsgPort *port)
{
    APTR SysBase = *(APTR *)4L;

    if (port->mp_Node.ln_Name)
        RemPort(port);
    FreeSignal(port->mp_SigBit);
    FreeMem(port,sizeof(*port));
}

struct IORequest *_autil_create_ext_io(struct MsgPort *port,LONG iosize)
{
    APTR SysBase = *(APTR *)4L;
    struct IORequest *ioreq = NULL;

    if (port && (ioreq=AllocMem(iosize,MEMF_CLEAR|MEMF_PUBLIC)))
    {
        ioreq->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
        ioreq->io_Message.mn_ReplyPort    = port;
        ioreq->io_Message.mn_Length       = iosize;
    }
    return ioreq;
}

struct IOStdReq *_autil_create_std_io(struct MsgPort *port)
{
    return (struct IOStdReq *)_autil_create_ext_io(port,sizeof(struct IOStdReq));
}

void _autil_delete_ext_io(struct IORequest *ioreq)
{
    APTR SysBase = *(APTR *)4L;
    LONG i;

    i = -1;
    ioreq->io_Message.mn_Node.ln_Type = i;
    ioreq->io_Device                  = (struct Device *)i;
    ioreq->io_Unit                    = (struct Unit *)i;
    FreeMem(ioreq,ioreq->io_Message.mn_Length);
}

void _autil_begin_io (struct IORequest *iorequest)
{
    register struct IORequest *a1 __asm("a1")=iorequest;
    register struct Device    *a6 __asm("a6")=iorequest->io_Device;

    __asm volatile ("jsr a6@(-30:W)" :: "r" (a1), "r" (a6));
}

void _autil_init(void)
{
	g_fp60 = SPFlt(60);
    g_fp50 = SPFlt(50);
}

void _autil_shutdown(void)
{
    DPRINTF ("_autil_shutdown: freeing memory...\n");
    while (g_mem)
    {
        AQB_memrec mem_next = g_mem->next;

#ifdef MEMDEBUG
        DPRINTF ("_autil_shutdown: freeing %ld bytes at 0x%08lx\n", g_mem->size, g_mem->mem);
        if (g_mem->marker1 != MARKER1)
        {
            DPRINTF("_autil_shutdown: *** ERROR: corruptet memlist, marker1 damaged\n");
            g_mem = mem_next;
            continue;
        }
        if (g_mem->marker2 != MARKER2)
        {
            DPRINTF("_autil_shutdown: *** ERROR: corruptet memlist, marker2 damaged\n");
            g_mem = mem_next;
            continue;
        }
        _MEMSET ((BYTE*)g_mem->mem, 0xEF, g_mem->size);
#endif
        FreeMem(g_mem->mem, g_mem->size);
        FreeMem(g_mem, sizeof (*g_mem));
        g_mem = mem_next;
    }
    DPRINTF ("_autil_shutdown: freeing memory... done.\n");
}

