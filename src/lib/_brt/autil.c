
//#define ENABLE_DPRINTF
#define MEMDEBUG

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

#ifdef MEMDEBUG
    #define MR_MEM_OFFSET (12+4)
    #define MR_OVERHEAD   (12+8)
#else // MEMDEBUG
    #define MR_MEM_OFFSET (12)
    #define MR_OVERHEAD   (12)
#endif // MEMDEBUG

typedef struct AQB_memrec_ *AQB_memrec;

struct AQB_memrec_
{
    AQB_memrec next, prev;
    ULONG      size;
#ifdef MEMDEBUG
    ULONG      marker1;
#endif
    ULONG      mem;
    // if MEMDEBUG, we have a ULONG marker2 after the mem block
};

#define MARKER1 0xAFFE1234
#define MARKER2 0xCAFEBABE

static AQB_memrec g_mem_first, g_mem_last = NULL;

_autil_sleep_for_cb_t _autil_sleep_for_cb = NULL;
static FLOAT g_fp50, g_fp60;

APTR ALLOCATE_(ULONG size, ULONG flags)
{

    DPRINTF ("ALLOCATE: size=%ld, flags=0x%08lx\n", size, flags);

    AQB_memrec mr = (AQB_memrec) AllocMem (size + MR_OVERHEAD, flags);
    if (!mr)
    {
        DPRINTF ("ALLOCATE_: OOM1\n");
        return NULL;
    }

    mr->next = NULL;
    mr->prev = g_mem_last;
    mr->size = size;

    if (g_mem_last)
        g_mem_last = g_mem_last->next = mr;
    else
        g_mem_first = g_mem_last = mr;

    BYTE *res = (BYTE *) &mr->mem;

#ifdef MEMDEBUG
    mr->marker1 = MARKER1;

    ULONG *m2 = (ULONG*)(res + size);
    *m2 = MARKER2;

    _MEMSET (res, 0xEF, size);
#endif

    DPRINTF ("ALLOCATE: res=0x%08lx, mr=0x%08lx\n", res, mr);

    return (APTR) res;
}

void DEALLOCATE (APTR ptr)
{
    BYTE *p = (BYTE*)ptr;
    AQB_memrec mr = (AQB_memrec) (p - MR_MEM_OFFSET);
    ULONG size = mr->size;

    DPRINTF ("DEALLOCATE: p=0x%08lx, mr=0x%08lx, size=%ld\n", p, mr, size);

#ifdef MEMDEBUG
    _AQB_ASSERT (mr->marker1 == MARKER1, (STRPTR) "DEALLOCATE: MARKER1 damaged");
    //DPRINTF ("DEALLOCATE: mr->marker1=0x%08lx\n", mr->marker1);
    ULONG *m2 = (ULONG*)(p + size);
    //DPRINTF ("DEALLOCATE: m2=0x%08lx\n", m2);
    _AQB_ASSERT (*m2 == MARKER2, (STRPTR) "DEALLOCATE: MARKER2 damaged");
    //DPRINTF ("DEALLOCATE: *m2=0x%08lx\n", *m2);
#endif

    if (mr->next)
        mr->next->prev = mr->prev;
    else
        g_mem_last = mr->prev;

    if (mr->prev)
        mr->prev->next = mr->next;
    else
        g_mem_first = mr->next;

#ifdef MEMDEBUG
    _MEMSET ((BYTE*)mr, 0xEF, size+MR_OVERHEAD);
#endif

    FreeMem ((APTR)mr, size+MR_OVERHEAD);
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

    __debug_puts(msg);
    __debug_puts((UBYTE *)"\n");

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
        __debug_puts((UBYTE*)"*** unhandled runtime error code: "); _DEBUG_PUTS2(errcode);
        __debug_puts((UBYTE*)"\n");
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

STRPTR TIME_ (void)
{
    struct DateStamp datetime;
    LONG             seconds;
    struct ClockData cd;

    DateStamp(&datetime);

    seconds = datetime.ds_Days*24*60*60 + datetime.ds_Minute*60 + datetime.ds_Tick/TICKS_PER_SECOND;

    DPRINTF ("TIME$: seconds=%ld\n", seconds);

    Amiga2Date (seconds, &cd);

    DPRINTF ("TIME$: %02d-%02d-%04d %02d:%02d:%02d\n", cd.month, cd.mday, cd.year, cd.hour, cd.min, cd.sec);

    char buf[8] = "00:00:00";

    buf[0] = '0' + cd.hour/10;
    buf[1] = '0' + cd.hour%10;

    buf[3] = '0' + cd.min/10;
    buf[4] = '0' + cd.min%10;

    buf[6] = '0' + cd.sec/10;
    buf[7] = '0' + cd.sec%10;

    return _astr_dup((STRPTR)buf);
}

STRPTR DATE_ (void)
{
    struct DateStamp datetime;
    LONG             seconds;
    struct ClockData cd;

	DateStamp(&datetime);

    seconds = datetime.ds_Days*24*60*60 + datetime.ds_Minute*60 + datetime.ds_Tick/TICKS_PER_SECOND;

    DPRINTF ("DATE$: datetime.ds_Days=%ld, datetime.ds_Minute=%ld, datetime.ds_Tick=%ld -> seconds=%ld\n",
             datetime.ds_Days, datetime.ds_Minute, datetime.ds_Tick, seconds);

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

/* The implementation here was originally done by Gary S. Brown.  I have
   borrowed the tables directly, and made some minor changes to the
   crc32-function (including changing the interface). //ylo */

  /* ============================================================= */
  /*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
  /*  code or tables extracted from it, as desired without restriction.     */
  /*                                                                        */
  /*  First, the polynomial itself and its table of feedback terms.  The    */
  /*  polynomial is                                                         */
  /*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
  /*                                                                        */
  /*  Note that we take it "backwards" and put the highest-order term in    */
  /*  the lowest-order bit.  The X^32 term is "implied"; the LSB is the     */
  /*  X^31 term, etc.  The X^0 term (usually shown as "+1") results in      */
  /*  the MSB being 1.                                                      */
  /*                                                                        */
  /*  Note that the usual hardware shift register implementation, which     */
  /*  is what we're using (we're merely optimizing it by doing eight-bit    */
  /*  chunks at a time) shifts bits into the lowest-order term.  In our     */
  /*  implementation, that means shifting towards the right.  Why do we     */
  /*  do it this way?  Because the calculated CRC must be transmitted in    */
  /*  order from highest-order term to lowest-order term.  UARTs transmit   */
  /*  characters in order from LSB to MSB.  By storing the CRC this way,    */
  /*  we hand it to the UART in the order low-byte to high-byte; the UART   */
  /*  sends each low-bit to hight-bit; and the result is transmission bit   */
  /*  by bit from highest- to lowest-order term without requiring any bit   */
  /*  shuffling on our part.  Reception works similarly.                    */
  /*                                                                        */
  /*  The feedback terms table consists of 256, 32-bit entries.  Notes:     */
  /*                                                                        */
  /*      The table can be generated at runtime if desired; code to do so   */
  /*      is shown later.  It might not be obvious, but the feedback        */
  /*      terms simply represent the results of eight shift/xor opera-      */
  /*      tions for all combinations of data and CRC register values.       */
  /*                                                                        */
  /*      The values must be right-shifted by eight bits by the "updcrc"    */
  /*      logic; the shift must be unsigned (bring in zeroes).  On some     */
  /*      hardware you could probably optimize the shift in assembler by    */
  /*      using byte-swap instructions.                                     */
  /*      polynomial $edb88320                                              */
  /*                                                                        */
  /*  --------------------------------------------------------------------  */

static unsigned long crc32_tab[] = {
      0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
      0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
      0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
      0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
      0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
      0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
      0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
      0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
      0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
      0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
      0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
      0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
      0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
      0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
      0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
      0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
      0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
      0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
      0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
      0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
      0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
      0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
      0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
      0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
      0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
      0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
      0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
      0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
      0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
      0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
      0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
      0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
      0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
      0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
      0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
      0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
      0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
      0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
      0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
      0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
      0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
      0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
      0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
      0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
      0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
      0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
      0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
      0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
      0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
      0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
      0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
      0x2d02ef8dL
   };

/* return a 32-bit CRC of the contents of the buffer. */
ULONG CRC32_ (const UBYTE *p, ULONG len)
{
    ULONG crc32val = 0;
    for (ULONG i = 0;  i < len;  i ++)
    {
        UBYTE c = p[i];
        crc32val = crc32_tab[(crc32val ^ c) & 0xff] ^ (crc32val >> 8);
    }
    return crc32val;
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
    AQB_memrec mr = g_mem_first;
    while (mr)
    {
        AQB_memrec mr_next = mr->next;
        ULONG size = mr->size;

        DPRINTF ("_autil_shutdown: freeing %ld+%ld bytes, mr=0x%08lx\n", size, MR_OVERHEAD, mr);
#ifdef MEMDEBUG
        if (mr->marker1 != MARKER1)
        {
            DPRINTF("_autil_shutdown: *** ERROR: corruptet memlist, marker1 damaged\n");
            mr = mr_next;
            continue;
        }
        BYTE *p = (BYTE*)&mr->mem;
        ULONG *m2 = (ULONG*)(p + size);
        if (*m2 != MARKER2)
        {
            DPRINTF("_autil_shutdown: *** ERROR: corruptet memlist, marker2 damaged\n");
            mr = mr_next;
            continue;
        }

        FreeMem ((APTR)mr, size+MR_OVERHEAD);
#endif
        mr = mr_next;
    }
    DPRINTF ("_autil_shutdown: freeing memory... done.\n");
}

