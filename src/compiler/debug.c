#include <assert.h>
#include <stdlib.h>

#include "ide.h"
#include "logger.h"
#include "ui.h"
#include "options.h"
#include "link.h"

#ifdef __amigaos__

#include "amigasupport.h"

#include <exec/execbase.h>
#include <dos/dostags.h>

#include <workbench/startup.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

#define TS_FROZEN          0xff

#define DEBUG_SIG          0xDECA11ED

#define DEBUG_CMD_START    23
#define DEBUG_CMD_PUTC     24
#define DEBUG_CMD_PUTS     25
#define DEBUG_CMD_TRAP     26

/* we send this instead of WBStartup when running a debug process */
struct DebugMsg
{
    struct Message  msg;
	struct MsgPort *port;
    ULONG           debug_sig;
    UWORD           debug_cmd;
    ULONG           debug_exitFn;
    union
    {
        ULONG   err;    // START return msg
        char    c;      // putc
        char   *str;    // puts
    }u;
};

typedef struct DEBUG_env_ *DEBUG_env;

struct DEBUG_env_
{
    DEBUG_state        state;
    struct Process    *childProc;
    char              *binfn;
    BPTR               childHomeDirLock;
    LI_segmentList     sl;
    BPTR               seglist;
    char               dirbuf[256];
    union
    {
        struct
        {
            struct DebugMsg    msg;
            ULONG              err;
        } dbg;
        struct
        {
            struct WBStartup   msg;
            struct WBArg       arg0;
            struct WBArg       arg1;
        } wb;
    } u;
};

#define DBG_EXC_BUF_LEN 128

struct dbgState
{
    ULONG   d0, d1, d2, d3, d4, d5, d6, d7;
    ULONG   a0, a1, a2, a3, a4, a5, a6, a7;

    UBYTE   exceptionData[DBG_EXC_BUF_LEN];
};

typedef struct DEBUG_stackInfo_ *DEBUG_stackInfo;

struct DEBUG_stackInfo_
{
    DEBUG_stackInfo next, prev;
    ULONG           pc, fp;
    int16_t         line;
    AS_frameMapNode fmn;
};

static IDE_instance         g_ide;
static struct MsgPort      *g_debugPort;
static struct Task         *g_parentTask;
static struct Task         *g_childTask;
static struct DEBUG_env_    g_dbgEnv   = {DEBUG_stateStopped, NULL, NULL, 0, 0};
static struct DEBUG_env_    g_helpEnv  = {DEBUG_stateStopped, NULL, NULL, 0, 0};
static ULONG                g_trapCode = 0;
static UWORD                g_dbgSR    = 0;
static ULONG                g_dbgPC    = 0;
static UWORD                g_dbgFMT   = 0;
static struct dbgState      g_dbgStateBuf;
static struct Message      *g_trapMsg   = NULL;
static BOOL                 g_terminate = FALSE;

static BOOL has_fpu         = FALSE;
static BOOL has_68060_or_up = FALSE;
static BOOL has_68040_or_up = FALSE;
static BOOL has_68030_or_up = FALSE;
static BOOL has_68020_or_up = FALSE;
static BOOL has_68010_or_up = FALSE;

// Usage:
//     _hexdump(addr, len, perLine);
//         addr:    the address to start dumping from.
//         len:     the number of bytes to dump.
//         perLine: number of bytes on each output line.

static void _hexdump (IDE_instance ed, const void * addr, const int len, int perLine)
{
    int i;
    unsigned char buff[perLine+1];
    const unsigned char * pc = (const unsigned char *)addr;

    for (i = 0; i < len; i++) {
        if ((i % perLine) == 0) {
            if (i != 0) IDE_cprintf (ed, "  %s\n", buff);
            IDE_cprintf (ed, "  %04x ", i);
        }

        IDE_cprintf (ed, " %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    while ((i % perLine) != 0) {
        IDE_cprintf (ed, "   ");
        i++;
    }

    IDE_cprintf (ed, "  %s\n", buff);
}

extern APTR _unfreeze_20, _unfreeze_00;

asm (
"       .text\n"
"       .align 2\n"
"__unfreeze_00:\n"

"       move.l  #_g_dbgStateBuf, a5;\n"  // from this point on, a5 points to cur location in _g_dbgStateBuf

	    // restore registers

"	    move.l	a5@+,d0;\n"
"	    move.l	a5@+,d1;\n"
"	    move.l	a5@+,d2;\n"
"	    move.l	a5@+,d3;\n"
"	    move.l	a5@+,d4;\n"
"	    move.l	a5@+,d5;\n"
"	    move.l	a5@+,d6;\n"
"	    move.l	a5@+,d7;\n"
"	    move.l	a5@+,a0;\n"
"	    move.l	a5@+,a1;\n"
"	    move.l	a5@+,a2;\n"
"	    move.l	a5@+,a3;\n"
"	    move.l	a5@+,a4;\n"
"	    move.l	a5@+,ssp@-;\n"
"	    move.l	a5@+,a6;\n"

"	    move.l	a5@+,a5;\n"     // a7
"	    move.l	a5,usp;\n"

"	    move.l	ssp@+, a5;\n"   // real a5

		// create a new stack frame with the original return address + sr
"       move.l  _g_dbgPC, ssp@-;\n"			// pc
"       move.w  _g_dbgSR, ssp@-;\n"         // sr

"       rte;\n"

"__unfreeze_20:\n"

"       move.l  #_g_dbgStateBuf, a5;\n"  // from this point on, a5 points to cur location in _g_dbgStateBuf

	    // restore registers

"	    move.l	a5@+,d0;\n"
"	    move.l	a5@+,d1;\n"
"	    move.l	a5@+,d2;\n"
"	    move.l	a5@+,d3;\n"
"	    move.l	a5@+,d4;\n"
"	    move.l	a5@+,d5;\n"
"	    move.l	a5@+,d6;\n"
"	    move.l	a5@+,d7;\n"
"	    move.l	a5@+,a0;\n"
"	    move.l	a5@+,a1;\n"
"	    move.l	a5@+,a2;\n"
"	    move.l	a5@+,a3;\n"
"	    move.l	a5@+,a4;\n"
"	    move.l	a5@+,ssp@-;\n"
"	    move.l	a5@+,a6;\n"

"	    move.l	a5@+,a5;\n"     // a7
"	    move.l	a5,usp;\n"

"	    move.l	ssp@+, a5;\n"   // real a5

		// create a new, format $0 stack frame with the original return address + sr
"       move.w  #0, ssp@-;\n"				// frame format
"       move.l  _g_dbgPC, ssp@-;\n"			// pc
"       move.w  _g_dbgSR, ssp@-;\n"         // sr

"       rte;\n"

"__debug_display_beep:\n"
"       move.l  d0, sp@-;\n"
"       moveq   #-1, d0;\n"
"_flash:\n"
"       move.l  d0, 0xdff180;\n"
"       dbra    d0, _flash;\n"
"       move.l  sp@+,d0;\n"
"       rts;\n"
);

void _debug_display_beep(void);

#if 0
static void _cflash(void)
{
    ULONG *p = (ULONG *)0xdff180;
    for (int cnt = 0; cnt<100; cnt++)
    {
        for (ULONG l=1; l++; l)
        {
            *p = l;
        }
    }
}
#endif

void _freeze_myself(void)
{
    // debug flash
    //_debug_display_beep();

	// we need a temporary reply port

    struct MsgPort *replyPort  = ASUP_create_port ((STRPTR) "AQB trap reply port", 0);
    if (!replyPort)
	{
		// FIXME!
		return;
	}

    // prepare debug message

    static struct DebugMsg   _dbgMsg;

	_dbgMsg.msg.mn_Node.ln_Succ = NULL;
	_dbgMsg.msg.mn_Node.ln_Pred = NULL;
	_dbgMsg.msg.mn_Node.ln_Pri  = 0;
	_dbgMsg.msg.mn_Node.ln_Name = NULL;
	_dbgMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
	_dbgMsg.msg.mn_Length       = sizeof(struct DebugMsg);
	_dbgMsg.msg.mn_ReplyPort    = replyPort;
	_dbgMsg.debug_sig           = DEBUG_SIG;
	_dbgMsg.debug_cmd           = DEBUG_CMD_TRAP;
	//_dbgMsg.u.trap              = g_trapCode;

	// and send it

	PutMsg (g_debugPort, &_dbgMsg.msg);

	// wait for a response
	WaitPort (replyPort);
	GetMsg(replyPort);

	// reply port is no longer needed
	ASUP_delete_port(replyPort);

	// continue execution of our program

	Supervisor(has_68010_or_up ? (void *)&_unfreeze_20 : (void *)&_unfreeze_00);
}

/*
 * origin: ixemul trap.S
 *
 * This is the trap processor for the mc68020 and above, paired with
 * an fpu (don't *need* an fpu though).
 *
 * idea: we save the complete cpu+fpu state, then jump into _freeze_myself
 *       via a manipulated pc + rte.
 *
 * later, if we decide to continue program execution, we restore
 * the complete cpu+fpu state and start from there.
 */

extern APTR _trap_handler_00;
extern APTR _trap_handler_20;

asm(
"       .text\n"
"       .align 2\n"
"__trap_handler_00:\n"

"       move.l  (ssp), _g_trapCode;\n" 	    // save trap code
"	    move.l  a5,(ssp);\n"		        // save a5

"       move.l  #_g_dbgStateBuf, a5;\n"     // from this point on, a5 points to cur location in _g_dbgStateBuf

	    // save registers in state buf
"	    move.l	d0,a5@+;\n"
"	    move.l	d1,a5@+;\n"
"	    move.l	d2,a5@+;\n"
"	    move.l	d3,a5@+;\n"
"	    move.l	d4,a5@+;\n"
"	    move.l	d5,a5@+;\n"
"	    move.l	d6,a5@+;\n"
"	    move.l	d7,a5@+;\n"
"	    move.l	a0,a5@+;\n"
"	    move.l	a1,a5@+;\n"
"	    move.l	a2,a5@+;\n"
"	    move.l	a3,a5@+;\n"
"	    move.l	a4,a5@+;\n"
"       move.l  ssp@+, a4;\n"	            // use real, saved a5 state
"	    move.l	a4,a5@+;\n"
"	    move.l	a6,a5@+;\n"
"	    move.l	usp, a0;\n"
"	    move.l	a0,a5@+;\n"

	    // now, save stack frame info
"       move.w  ssp@+,_g_dbgSR;\n"	        // sr
"       move.l	ssp@+,_g_dbgPC;\n"	        // pc

		// create a new stack frame with a manipulated return address
"       move.l  #__freeze_myself, ssp@-;\n" // pc
"       move.w  #0, ssp@-;\n"               // sr

"       rte;\n"

"__trap_handler_20:\n"

"       move.l  (ssp), _g_trapCode;\n" 	    // save trap code
"	    move.l  a5,(ssp);\n"		        // save a5

"       move.l  #_g_dbgStateBuf, a5;\n"     // from this point on, a5 points to cur location in _g_dbgStateBuf

	    // save registers in state buf
"	    move.l	d0,a5@+;\n"
"	    move.l	d1,a5@+;\n"
"	    move.l	d2,a5@+;\n"
"	    move.l	d3,a5@+;\n"
"	    move.l	d4,a5@+;\n"
"	    move.l	d5,a5@+;\n"
"	    move.l	d6,a5@+;\n"
"	    move.l	d7,a5@+;\n"
"	    move.l	a0,a5@+;\n"
"	    move.l	a1,a5@+;\n"
"	    move.l	a2,a5@+;\n"
"	    move.l	a3,a5@+;\n"
"	    move.l	a4,a5@+;\n"
"       move.l  ssp@+, a4;\n"	            // use real, saved a5 state
"	    move.l	a4,a5@+;\n"
"	    move.l	a6,a5@+;\n"
"	    move.l	usp, a0;\n"
"	    move.l	a0,a5@+;\n"

	    // now, save stack frame info
"       move.w  ssp@+,_g_dbgSR;\n"	        // sr
"       move.l	ssp@+,_g_dbgPC;\n"	        // pc
"       move.l	ssp@+,_g_dbgFMT;\n"	        // frame format

		// create a new, format $0 stack frame with a manipulated return address
"       move.w  #0, ssp@-;\n"				// frame format
"       move.l  #__freeze_myself, ssp@-;\n" // pc
"       move.w  #0, ssp@-;\n"               // sr

#if 0
// ---------------------------------------------------------------
// FIXME: debug flash code, remove
"       move.l  d0, ssp@-;\n"
"       moveq   #-1, d0;\n"
"_flash:\n"
"       move.l  d0, 0xdff180;\n"
"       dbra    d0, _flash;\n"
"       move.l  ssp@+,d0;\n"
// FIXME: debug flash code, remove
// ---------------------------------------------------------------
#endif

"       rte;\n"
);

#if 0

"	movel	a5,sp@		| nuke the trap number, we use the frame format word\n"
"	movel	usp,a5		| get usp\n"
"	movel	a5,a5@(-10)	| store usp\n"
"	lea	a5@(-10),a5	| make room for sr, pc and usp.\n"
"	moveml	d0-d7/a0-a6,a5@-| store registers on usp\n"
"	movel	sp@+,a5@(0x34)	| insert the saved a5 into the saveset\n"
"	movew	sp@+,a5@(0x44)	| copy SR\n"
"	movel	sp@+,d2		| remember and\n"
"	movel	d2,a5@(0x40)	| copy (offending?) PC\n"
"	movel	a5,a3		| save pointer to registers\n"
"\n"
"	| find out more about the frame (according to the MC68030 user manual)\n"
"	clrl	d1\n"
"	movew	sp@+,d1		| remember frame format word\n"
"	movew	d1,d0\n"
"	andw	#0xf000,d0\n"
"	beq	Lfmt_S0		| S0\n"
"	cmpw	#0x1000,d0\n"
"	beq	Lfmt_S1		| S1 this (interrupt) frame shouldn't be here...\n"
"	cmpw	#0x2000,d0\n"
"	beq	Lfmt_S2		| CHK{2},cpTRAPcc,TRAPV,Trace,Div0,MMUcfg,cp post instr\n"
"	cmpw	#0x9000,d0\n"
"	beq	Lfmt_S9		| cp mid instr,main det prot viol,int during cp instr\n"
"	cmpw	#0xa000,d0\n"
"	beq	Lfmt_SA_SB	| address or bus error, short and long frame\n"
"	cmpw	#0xb000,d0\n"
"	bne	Lfmt_S0		| ??? frame, this will probably not fully cleanup sp..\n"
"\n"
"Lfmt_SA_SB:\n"
"	| this part (upto Lbe10) inspired by locore.s in sys/hp300/ of BSD4.3-reno\n"
"	movew	sp@(2),d0	| grab SSW for fault processing\n"
"	btst	#12,d0		| RB set?\n"
"	beq	LbeX0		| no, test RC\n"
"	bset	#14,d0		| yes, must set FB\n"
"	movew	d0,sp@(2)	| for hardware too\n"
"LbeX0:\n"
"	btst	#13,d0		| RC set?\n"
"	beq	LbeX1		| no, skip\n"
"	bset	#15,d0		| yes, must set FC\n"
"	movew	d0,sp@(2)	| for hardware too\n"
"LbeX1:\n"
"	btst	#8,d0		| data fault?\n"
"	beq	Lbe0		| no, check for hard cases\n"
"	movel	sp@(8),d2	| fault address is as given in frame\n"
"	bra	Lbe10		| thats it\n"
"Lbe0:\n"
"	btst	#12,d1		| long (type B) stack frame?\n"
"	bne	Lbe4		| yes, go handle\n"
"	btst	#14,d0		| no, can use saved PC. FB set?\n"
"	beq	Lbe3		| no, try FC\n"
"	addql	#4,d2		| yes, adjust address\n"
"	bra	Lbe10		| done\n"
"Lbe3:\n"
"	btst	#15,d0		| FC set?\n"
"	beq	Lbe10		| no, done\n"
"	addql	#2,d2		| yes, adjust address\n"
"	bra	Lbe10		| done\n"
"Lbe4:\n"
"	movel	sp@(28),d2	| long format, use stage B address\n"
"	btst	#15,d0		| FC set?\n"
"	beq	Lbe10		| no, all done\n"
"	subql	#2,d2		| yes, adjust address\n"
"Lbe10:\n"
"\n"
"	| now move the frame over to the usp (6/21 longwords remain)\n"
"	\n"
"	moveml	sp@+,d3-d7/a0	| may trash as many registers as I like, I saved\n"
"	moveml	d3-d7/a0,a5@-	| them already ;-) First copy 6 longs\n"
"\n"
"	btst	#12,d1		| long (type B) stack frame?\n"
"	beq	Lfmt_S0		| nope, done\n"
"\n"
"	moveml	sp@+,d3-d7/a0-a2 | first copy 8 longs\n"
"	moveml	d3-d7/a0-a2,a5@-\n"
"	moveml	sp@+,d3-d7/a0-a1 | plus 7 gives 15, plus already stored 6 is 21\n"
"	moveml	d3-d7/a0-a1,a5@-\n"
"	bra	Lfmt_S0		| finito\n"
"\n"
"Lfmt_S9:\n"
"	movel	sp@+,a5@-	| S9 is an S2 plus 4 internal (word length) registers\n"
"	movel	sp@+,a5@-	| so store those registers, and fall into S2\n"
"\n"
"Lfmt_S2:\n"
"	movel	sp@+,d2		| S2 contains the offending instruction address\n"
"				| and the frame format word\n"
"	movel	d2,a5@-		| we have the offending instruction address here\n"
"\n"
"	| fall into\n"
"\n"
"Lfmt_S0:\n"
"Lfmt_S1:\n"
"	movew	d1,a5@-		| and as the last thing store the frame format word\n"
"\n"
"	|\n"
"	| now lets look at the fpu, if there is an fpu in the system\n"
"	|\n"
"\n"
"	movel	#0,a4		| clear pointer to fpu registers\n"
"	tstl    _has_fpu        | do we have a fpu?\n"
"	beq	Lno_fpu\n"
"	fsave	a5@-		| dump the fpu state onto the usp\n"
"	moveb	a5@,d0		| and get the fpu state identifier\n"
"	beq	Lno_fpu		| null frame?\n"
"\n"
"	fmoveml	fpcr/fpsr/fpi,a5@-	| push the fpu control registers and\n"
"	fmovemx	fp0-fp7,a5@-		| the fpu data registers\n"
"	movel	a5,a4		| store pointer to fpu registers\n"
"\n"
"	movew	#-1,a5@-	| mark that there is fpu stuff on the stack\n"
"Lno_fpu:\n"
"\n"
"	|\n"
"	| pass return address and ssp-value on userstack\n"
"	| This happens for the same reason as we have a glue_launch entry.\n"
"	| trap cleans up these 8 bytes on the user stack itself\n"
"	movel	a4,a5@-		| pass pointer to stored fpu registers\n"
"	movel	a3,a5@-		| pass pointer to stored registers\n"
"	movel	d2,a5@-		| pass offending PC\n"
"	movel	d1,a5@-		| pass frame format word\n"
"\n"
"	movel	sp,a5@-\n"
"	movel	#_restore_20,a5@-\n"
"\n"
"	movel	a5,usp		| set the new value of the usp\n"
"\n"
"	| that's it, phew.. now process this frame, and perhaps throw some\n"
"	| frames on it as well to deal with the signal\n"

#endif

static void dumpSegmentList(BPTR seglist)
{
    ULONG *sl = BADDR(seglist);

    LOG_printf (LOG_DEBUG, "dumpSegmentList: seglist=0x%08lx sl=0x%08lx\n", seglist, sl);
    while (sl)
    {
        uint16_t *p = (uint16_t*)(sl+1);
        LOG_printf (LOG_DEBUG, "dumpSegmentList: SEGMENT 0x%08lx size=%5d next=0x%08lx bytes: 0x%04x 0x%04x 0x%04x 0x%04x\n", sl, *(sl-1), *sl, p[0], p[1], p[2], p[3]);
        sl = BADDR(*sl);
    }
}

static LI_segmentList _loadSeg(char *binfn)
{
    LOG_printf (LOG_INFO, "Loading %s ...\n", binfn);

    FILE *f = fopen (binfn, "r");
    if (!f)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s\n\n", binfn);
        return 0;
    }

    LI_segmentList sl = LI_SegmentList();

    if (!LI_segmentListReadLoadFile (UP_runChild, sl, binfn, f))
    {
        fclose (f);
        LOG_printf (LOG_ERROR, "*** ERROR: failed to read %s\n\n", binfn);
        return 0;
    }
    fclose (f);

    LI_relocate (sl);
    //LOG_printf (LOG_INFO, "\nhex dump: beginning of first segment\n");
    //_hexdump ((UBYTE *)sl->first->seg->mem, /*len=*/32, /*perLine=*/16);

    // create AmigaDOS style seglist

    for (LI_segmentListNode sln=sl->first; sln; sln=sln->next)
    {
        AS_segment seg = sln->seg;

        uint32_t *ptr = (uint32_t *) seg->mem;
        ptr -= 2;
        ptr[0] = (uint32_t) seg->mem_pos+8;
        if (sln->next)
        {
            ptr[1] = MKBADDR((uint32_t)sln->next->seg->mem - 4);
        }
        else
        {
            ptr[1] = 0;
        }
        LOG_printf (LOG_DEBUG, "_loadSeg: creating seglist: kind=%d, hunk_id=%d, next=0x%08lx -> len=%6d next=0x%08lx code:0x%08lx...\n",
                    seg->kind, seg->hunk_id, sln->next, ptr[0], ptr[1], ptr[2]);
        //U_delay(1000);
    }
    LOG_printf (LOG_DEBUG, "_loadSeg: creating seglist: done.\n");
    //U_delay(1000);

    // clear cpu caches
    CacheClearU();

    LOG_printf (LOG_DEBUG, "_loadSeg: done.\n");

    return sl;
}

static bool _launch_process (DEBUG_env env, char *binfn, char *arg1, bool dbg)
{
    env->binfn = binfn;

    LOG_printf (LOG_DEBUG, "RUN _launch_process: loading %s ...\n\n", binfn);

    // use our custom loader which handles debug info
    env->sl = _loadSeg(binfn);
    if (!env->sl || !env->sl->first)
        return FALSE;
    env->seglist = MKBADDR(env->sl->first->seg->mem)-1;
    dumpSegmentList(env->seglist);

    LOG_printf (LOG_INFO, "Running %s ...\n\n", binfn);

    // homedir

    strncpy (env->dirbuf, binfn, 256);
    *(PathPart((STRPTR)env->dirbuf)) = 0;

    env->childHomeDirLock = Lock ((STRPTR)env->dirbuf, ACCESS_READ);

    LOG_printf (LOG_DEBUG, "RUN _launch_process: CreateNewProc for %s ...\n", binfn);
    env->childProc = CreateNewProcTags(NP_Seglist,     (ULONG) env->seglist,
									   NP_FreeSeglist, FALSE,
									   NP_Input,       0l,
                                       NP_Output,      aqb_wbstart ? 0 : Output(),
                                       NP_CloseInput,  FALSE,
                                       NP_CloseOutput, FALSE,
                                       NP_StackSize,   DEFAULT_STACKSIZE,
								       NP_Name,        (ULONG) binfn,
									   //NP_WindowPtr,   0l,
									   NP_HomeDir,     env->childHomeDirLock,
									   NP_CopyVars,    FALSE,
									   TAG_DONE);

    g_childTask = &env->childProc->pr_Task;

    LOG_printf (LOG_DEBUG, "RUN _launch_process: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) env->childProc);

    // send startup message

    if (dbg)
    {

        // install trap handler first
        env->childProc->pr_Task.tc_TrapCode = has_68010_or_up ? (APTR) &_trap_handler_20 : (APTR) &_trap_handler_00;

#if 0
        // insert a breakpoint right at the start

        {
            uint16_t *ptr = (uint16_t *) sl->first->seg->mem;
            LOG_printf (LOG_DEBUG, "RUN _launch_process: injecting breakpoint at 0x%08lx: 0x%04lx->0x%04lx\n", ptr, *ptr, 0x4e41);
            *ptr = 0x4e41; // trap #1
            CacheClearU();
        }
#endif

        env->u.dbg.msg.msg.mn_Node.ln_Succ = NULL;
        env->u.dbg.msg.msg.mn_Node.ln_Pred = NULL;
        env->u.dbg.msg.msg.mn_Node.ln_Type = NT_MESSAGE;
        env->u.dbg.msg.msg.mn_Node.ln_Pri  = 0;
        env->u.dbg.msg.msg.mn_Node.ln_Name = NULL;
        env->u.dbg.msg.msg.mn_ReplyPort    = g_debugPort;
        env->u.dbg.msg.msg.mn_Length       = sizeof(struct DebugMsg);
        env->u.dbg.msg.port                = &env->childProc->pr_MsgPort;
        env->u.dbg.msg.debug_sig           = DEBUG_SIG;
        env->u.dbg.msg.debug_cmd           = DEBUG_CMD_START;
        env->u.dbg.err                     = 0;

        LOG_printf (LOG_DEBUG, "RUN _launch_process: Send debug msg, g_debugPort=0x%08lx...\n", g_debugPort);

        PutMsg (&env->childProc->pr_MsgPort, &env->u.dbg.msg.msg);
    }
    else
    {
        env->u.wb.msg.sm_Message.mn_Node.ln_Succ = NULL;
        env->u.wb.msg.sm_Message.mn_Node.ln_Pred = NULL;
        env->u.wb.msg.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
        env->u.wb.msg.sm_Message.mn_Node.ln_Pri  = 0;
        env->u.wb.msg.sm_Message.mn_Node.ln_Name = NULL;
        env->u.wb.msg.sm_Message.mn_ReplyPort    = g_debugPort;
        env->u.wb.msg.sm_Message.mn_Length       = sizeof(struct WBStartup);
        env->u.wb.msg.sm_Process                 = &env->childProc->pr_MsgPort;
        env->u.wb.msg.sm_Segment                 = env->seglist;
        env->u.wb.msg.sm_NumArgs                 = arg1 ? 2 : 1;
        env->u.wb.msg.sm_ToolWindow              = NULL;
        env->u.wb.msg.sm_ArgList                 = &env->u.wb.arg0;

        env->u.wb.arg0.wa_Lock = Lock ((STRPTR)env->dirbuf, ACCESS_READ);
        env->u.wb.arg0.wa_Name = (BYTE*) FilePart ((STRPTR)binfn);

        if (arg1)
        {
            strncpy (env->dirbuf, arg1, 256);
            *(PathPart((STRPTR)env->dirbuf)) = 0;

            env->u.wb.arg1.wa_Lock = Lock ((STRPTR)env->dirbuf, ACCESS_READ);
            env->u.wb.arg1.wa_Name = (BYTE*) FilePart ((STRPTR)arg1);
        }
        LOG_printf (LOG_DEBUG, "RUN _launch_process: Send WBSTartup msg (arg1=%s) ...\n", arg1);

        PutMsg (&env->childProc->pr_MsgPort, &env->u.wb.msg.sm_Message);
    }

    env->state = DEBUG_stateRunning;

	LOG_printf (LOG_DEBUG, "RUN _launch_process: done. state is %d now.\n", env->state);

    return TRUE;
}

bool DEBUG_start (const char *binfn)
{
    return _launch_process (&g_dbgEnv, (char *)binfn, /*arg=*/NULL, /*dbg=*/TRUE);
}

void DEBUG_help (char *binfn, char *arg1)
{
    if (g_helpEnv.state != DEBUG_stateStopped)
    {
        LOG_printf (LOG_ERROR, "help viewer is already active.\n");
        return;
    }

    g_helpEnv.binfn = binfn;

    LOG_printf (LOG_DEBUG, "DEBUG_help: loading %s ...\n\n", binfn);

    g_helpEnv.sl = NULL;
    g_helpEnv.seglist = LoadSeg ((STRPTR)binfn);
    if (!g_helpEnv.seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", binfn);
        return;
    }

    // homedir

    strncpy (g_helpEnv.dirbuf, binfn, 256);
    *(PathPart((STRPTR)g_helpEnv.dirbuf)) = 0;

    g_helpEnv.childHomeDirLock = Lock ((STRPTR)g_helpEnv.dirbuf, ACCESS_READ);

    LOG_printf (LOG_DEBUG, "DEBUG_help: CreateNewProc for %s ...\n", binfn);
    g_helpEnv.childProc = CreateNewProcTags(NP_Seglist,     (ULONG) g_helpEnv.seglist,
									   NP_FreeSeglist, FALSE,
									   NP_Input,       0l,
                                       NP_Output,      aqb_wbstart ? 0 : Output(),
                                       NP_CloseInput,  FALSE,
                                       NP_CloseOutput, FALSE,
                                       NP_StackSize,   DEFAULT_STACKSIZE,
								       NP_Name,        (ULONG) binfn,
									   //NP_WindowPtr,   0l,
									   NP_HomeDir,     g_helpEnv.childHomeDirLock,
									   NP_CopyVars,    FALSE,
									   TAG_DONE);

    g_childTask = &g_helpEnv.childProc->pr_Task;

    LOG_printf (LOG_DEBUG, "DEBUG_help: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) g_helpEnv.childProc);

    // send startup message

    g_helpEnv.u.wb.msg.sm_Message.mn_Node.ln_Succ = NULL;
    g_helpEnv.u.wb.msg.sm_Message.mn_Node.ln_Pred = NULL;
    g_helpEnv.u.wb.msg.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
    g_helpEnv.u.wb.msg.sm_Message.mn_Node.ln_Pri  = 0;
    g_helpEnv.u.wb.msg.sm_Message.mn_Node.ln_Name = NULL;
    g_helpEnv.u.wb.msg.sm_Message.mn_ReplyPort    = g_debugPort;
    g_helpEnv.u.wb.msg.sm_Message.mn_Length       = sizeof(struct WBStartup);
    g_helpEnv.u.wb.msg.sm_Process                 = &g_helpEnv.childProc->pr_MsgPort;
    g_helpEnv.u.wb.msg.sm_Segment                 = g_helpEnv.seglist;
    g_helpEnv.u.wb.msg.sm_NumArgs                 = arg1 ? 2 : 1;
    g_helpEnv.u.wb.msg.sm_ToolWindow              = NULL;
    g_helpEnv.u.wb.msg.sm_ArgList                 = &g_helpEnv.u.wb.arg0;

    g_helpEnv.u.wb.arg0.wa_Lock = Lock ((STRPTR)g_helpEnv.dirbuf, ACCESS_READ);
    g_helpEnv.u.wb.arg0.wa_Name = (BYTE*) FilePart ((STRPTR)binfn);

    if (arg1)
    {
        strncpy (g_helpEnv.dirbuf, arg1, 256);
        *(PathPart((STRPTR)g_helpEnv.dirbuf)) = 0;

        g_helpEnv.u.wb.arg1.wa_Lock = Lock ((STRPTR)g_helpEnv.dirbuf, ACCESS_READ);
        g_helpEnv.u.wb.arg1.wa_Name = (BYTE*) FilePart ((STRPTR)arg1);
    }
    LOG_printf (LOG_DEBUG, "DEBUG_help: Send WBSTartup msg (arg1=%s) ...\n", arg1);

    PutMsg (&g_helpEnv.childProc->pr_MsgPort, &g_helpEnv.u.wb.msg.sm_Message);

    g_helpEnv.state = DEBUG_stateRunning;

	LOG_printf (LOG_DEBUG, "DEBUG_help: done. state is %d now.\n", g_helpEnv.state);
}

static void _find_debug_info (uint32_t pc, int16_t *l, AS_frameMapNode *fmn)
{
    *l = -1;
    *fmn = NULL;
    for (LI_segmentListNode sln=g_dbgEnv.sl->first; sln; sln=sln->next)
    {
        ULONG seg_start = (uint32_t) (uintptr_t) sln->seg->mem;
        ULONG seg_end   = seg_start + sln->seg->mem_size;

        LOG_printf (LOG_DEBUG, "_find_debug_info: looking for segment, pc=0x%08lx seg_start=0x%08lx seg_end=0x%08lx kind=%d srcMap=0x%08lx\n",
                    pc, seg_start, seg_end, sln->seg->kind, sln->seg->srcMap);

        if ((pc<seg_start) || (pc>=seg_end))
            continue;
        LOG_printf (LOG_DEBUG, "_find_debug_info: segment matched.\n");

        for (AS_srcMapNode n = sln->seg->srcMap; n; n=n->next)
        {
            LOG_printf (LOG_DEBUG, "_find_debug_info: looking for source line, pc=0x%08lx n->offset=0x%08lx n->line=%d -> l=%d\n", pc, n->offset, n->line, l);

            if (pc > n->offset)
                *l = n->line;
        }

        for (AS_frameMapNode n = sln->seg->frameMap; n; n=n->next)
        {
            LOG_printf (LOG_DEBUG, "_find_debug_info: looking for frame, pc=0x%08lx n->code_start=0x%08lx n->code_end=0x%08lx -> label=%s\n",
                        pc, n->code_start, n->code_end, *fmn ? S_name((*fmn)->label) : "NULL");

            if ((pc >= n->code_start) && (pc < n->code_end))
                *fmn = n;
        }
    }

}

static BOOL _getParentFrame (uint32_t *a5, uint32_t *pc)
{
    struct Task *task = &g_dbgEnv.childProc->pr_Task;
    uint32_t sp_lower = (uint32_t) task->tc_SPLower;
    uint32_t sp_upper = (uint32_t) task->tc_SPUpper;

    uint32_t spn = *a5;
    //IDE_cprintf ("_getParentFrame: spn=0x%08lx\n", spn);

    // is this a valid stack pointer ?

    if ( (spn % 2) || (spn<sp_lower) || (spn>sp_upper) )
    {
        //IDE_cprintf ("_getParentFrame: sp 0x%08lx is invalid (stack bounds: 0x%08lx-0x%08lx)\n", spn, sp_lower, sp_upper);
        return FALSE;
    }

    uint32_t *sp = (uint32_t *) spn;

    //_hexdump ((UBYTE *)sp, 32, 16);

    uint32_t prev_a5 = *sp++;
    uint32_t prev_pc = *sp++;

    //LOG_printf (LOG_DEBUG, "_getParentFrame: a5=0x%08lx -> prev_a5=0x%08lx, prev_pc=0x%08lx\n", *a5, prev_a5, prev_pc);

    *a5 = prev_a5;
    *pc = prev_pc;

    return TRUE;
}

static DEBUG_stackInfo DEBUG_StackInfo (ULONG pc, ULONG fp, int16_t line, AS_frameMapNode fmn)
{
    DEBUG_stackInfo si = U_poolAlloc (UP_runChild, sizeof (*si));

    si->prev = NULL;
    si->next = NULL;
    si->pc   = pc;
    si->fp   = fp;
    si->line = line;
    si->fmn  = fmn;

    return si;
}

static void _print_stack(IDE_instance ed, DEBUG_stackInfo si, DEBUG_stackInfo si_cur)
{
    IDE_cprintf (ed, "stack trace:\n\n");
    while (si)
    {
        if (si == si_cur)
            UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_TEXT);
        else
            UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_COMMENT);

        if (si->line>=0)
        {
            IDE_cprintf (ed, "%s 0x%08lx %s:%d %s\n",
                         si==si_cur ? "-->" : "   ",
                         si->pc, g_ide->sourcefn, si->line,
                         si->fmn ? S_name(si->fmn->label) : "???");
        }
        else
        {
            IDE_cprintf (ed, "%s 0x%08lx runtime/os\n", si==si_cur ? "-->" : "   ", si->pc);
        }
        si = si->next;
    }

    UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_TEXT);
    IDE_cprintf (ed, "\n\n");
}

static void _print_variable (IDE_instance ed, AS_frameVarNode v, uint8_t *fp)
{
    uint8_t *p = fp + v->offset;
    switch (v->ty->kind)
    {
        case Ty_bool:     IDE_cprintf (ed, "  BOOL     %-12s = %s\n", S_name(v->sym), *p ? "TRUE" : "FALSE"); break;
        case Ty_byte:     IDE_cprintf (ed, "  BYTE     %-12s = %d\n", S_name(v->sym), *p);                    break;
        case Ty_ubyte:    IDE_cprintf (ed, "  UBYTE    %-12s = %d\n", S_name(v->sym), *((uint8_t *)p));       break;
        case Ty_integer:  IDE_cprintf (ed, "  INTEGER  %-12s = %d\n", S_name(v->sym), *((int16_t *)p));       break;
        case Ty_uinteger: IDE_cprintf (ed, "  UINTEGER %-12s = %d\n", S_name(v->sym), *((uint16_t *)p));      break;
        case Ty_long:     IDE_cprintf (ed, "  LONG     %-12s = %d\n", S_name(v->sym), *((int32_t *)p));       break;
        case Ty_ulong:    IDE_cprintf (ed, "  ULONG    %-12s = %d\n", S_name(v->sym), *((uint32_t *)p));      break;
        case Ty_single:   IDE_cprintf (ed, "  SINGLE   %-12s = %f\n", S_name(v->sym), decode_ffp(*((uint32_t *)p))); break;

        case Ty_double:
        case Ty_sarray:
        case Ty_darray:
        case Ty_record:
        case Ty_pointer:
        case Ty_string:
        case Ty_void:
        case Ty_forwardPtr:
        case Ty_procPtr:
        case Ty_toLoad:
        case Ty_prc:
            IDE_cprintf (ed, "%s (%2d) %d\n", S_name(v->sym), v->ty->kind, v->offset);
    }

}

static void _print_variables(IDE_instance ed, DEBUG_stackInfo si)
{
    if (!si || !si->fmn || !si->fmn->vars)
        return;

    AS_frameVarNode v = si->fmn->vars;
    uint8_t *fp = (uint8_t *) si->fp;

    IDE_cprintf (ed, "variables:\n\n");
    while (v)
    {
        _print_variable (ed, v, fp);

        v = v->next;
    }

    for (LI_segmentListNode sln=g_dbgEnv.sl->first; sln; sln=sln->next)
    {
        for (AS_globalVarNode n = sln->seg->globals; n; n=n->next)
        {
            IDE_cprintf (ed, " global var: %s\n", S_name (n->sym));
        }
    }

    IDE_cprintf (ed, "\n\n");
}

static void _goto_line (IDE_instance ed, DEBUG_stackInfo si)
{
    if (!si)
        return;

    int16_t l = si->line;
    if (l<0)
        return;

    IDE_gotoLine (ed, l, 1, /*hilight=*/TRUE);
}

// FIXME: remove
#if 0
static void _print_listing (IDE_instance ed, DEBUG_stackInfo si)
{
    if (!si)
        return;

    int16_t l = si->line;
    if (l<0)
    {
        // FIXME: disassembly ?

        _hexdump (ed, (UBYTE *)si->pc, 32, 16);
        IDE_cprintf (ed, "\n");
        return;
    }

    // list source code context:

    IDE_cprintf (ed, "source code listing:\n\n");
    for (int ln = l-5; ln<l+5; ln++)
    {
        if (ln<0)
            continue;

        IDE_line line = IDE_getALine (g_ide, ln);
        if (!line)
            continue;

        if (l==ln+1)
        {
            UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_TEXT);
            IDE_cprintf (ed, "--> ");
        }
        else
        {
            UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_COMMENT);
            IDE_cprintf (ed, "    ");
        }

        for (int8_t i = 0; i<line->indent; i++)
        {
            for (int8_t j = 0; j<INDENT_SPACES; j++)
            {
                IDE_cprintf (ed, " ");
            }
        }
        IDE_cprintf (ed, "%s\n", line->buf);
    }
    UI_setTextStyle (ed->view_console, UI_TEXT_STYLE_TEXT);
    IDE_cprintf (ed, "\n");
}
#endif

#define MAX_NUM_PARTS 10

static BOOL is_whitespace(char c)
{
    return c<=32;
}

static void _debug(struct DebugMsg *msg)
{
    LOG_printf (LOG_DEBUG, "_debug: starts...\n");
    UI_toFront();
    IDE_cprintf (g_ide, "\n");

    UI_setTextStyle (g_ide->view_console, UI_TEXT_STYLE_KEYWORD);
    switch (g_trapCode)
    {
        case 2:
            IDE_cprintf(g_ide, "ACCESS FAULT\n\n");
            break;
        case 3:
            IDE_cprintf(g_ide, "ADDRESS ERROR\n\n");
            break;
        case 4:
            IDE_cprintf(g_ide, "ILLEGAL INSTRUCTION\n\n");
            break;
        case 5:
            IDE_cprintf(g_ide, "INTEGER DIVIDE BY ZERO\n\n");
            break;
        case 9:
            IDE_cprintf(g_ide, "TRACE SINGLE STEP\n\n");
            break;
        case 32:
            IDE_cprintf(g_ide, "CTRL-C BREAK\n\n");
            break;
        case 33:
            IDE_cprintf(g_ide, "BREAKPOINT HIT\n\n");
            break;
        case 34:
            IDE_cprintf(g_ide, "ASSERTION FAILED\n\n");
            break;
        case 35:
            IDE_cprintf(g_ide, "RUNTIME ERROR %d ", g_dbgEnv.u.dbg.msg.u.err);
            switch (g_dbgEnv.u.dbg.msg.u.err)
            {
                case 101: IDE_cprintf(g_ide, "(WIN OPEN)"); break;
                case 102: IDE_cprintf(g_ide, "(SCREEN OPEN)"); break;
                case 103: IDE_cprintf(g_ide, "(PALETTE)"); break;
                case 104: IDE_cprintf(g_ide, "(COLOR)"); break;
                case 105: IDE_cprintf(g_ide, "(AREA)"); break;
                case 106: IDE_cprintf(g_ide, "(PATTERN)"); break;
                case 107: IDE_cprintf(g_ide, "(WIN CLOSE)"); break;
                case 108: IDE_cprintf(g_ide, "(WIN OUTPUT)"); break;
                case 109: IDE_cprintf(g_ide, "(SCREEN CLOSE)"); break;
                case 110: IDE_cprintf(g_ide, "(PAINT)"); break;
                case 111: IDE_cprintf(g_ide, "(LINE)"); break;
                case 112: IDE_cprintf(g_ide, "(PSET)"); break;
                case 113: IDE_cprintf(g_ide, "(INPUT OUT OF DATA)"); break;
                case 114: IDE_cprintf(g_ide, "(ON TIMER CALL)"); break;
                case 115: IDE_cprintf(g_ide, "(TIMER ON)"); break;
                case 116: IDE_cprintf(g_ide, "(TIMER OFF)"); break;
                case 117: IDE_cprintf(g_ide, "(OPEN)"); break;
                case 118: IDE_cprintf(g_ide, "(OUTPUT)"); break;
                case 119: IDE_cprintf(g_ide, "(CLOSE)"); break;
                case 120: IDE_cprintf(g_ide, "(MOUSE)"); break;
                case 121: IDE_cprintf(g_ide, "(BLIT)"); break;
                case 200: IDE_cprintf(g_ide, "(IFF)"); break;
                case 300: IDE_cprintf(g_ide, "(GELS INIT)"); break;
                case 301: IDE_cprintf(g_ide, "(BOB)"); break;
            }
            IDE_cprintf(g_ide, "\n\n");
            break;
        default:
            IDE_cprintf(g_ide, "TRAP #%d (\?\?\?) occured.\n\n", g_trapCode);
    }
    UI_setTextStyle (g_ide->view_console, UI_TEXT_STYLE_TEXT);

    // get stack trace

    LOG_printf (LOG_DEBUG, "_debug: get stack trace...\n");

    DEBUG_stackInfo stack_first = NULL, stack_last = NULL;

    uint32_t pc = g_dbgPC;
    uint32_t a5 = g_dbgStateBuf.a5;
    int cnt = 0;
    while ( TRUE )
    {
        int16_t l;
        AS_frameMapNode fmn;

        _find_debug_info (pc, &l, &fmn);
        DEBUG_stackInfo si = DEBUG_StackInfo (pc, a5, l, fmn);
        si->prev = stack_last;
        if (!stack_first)
            stack_last = stack_first = si;
        else
            stack_last = stack_last->next = si;

        //IDE_cprintf (g_ide, "stack: pc=0x%08lx a5=0x%08lx -> source line = %d\n", pc, a5, l);
        if (!_getParentFrame(&a5, &pc))
            break;

        cnt++;
        if (cnt>10)
            break;
    }

    // determine current stack frame (first one we have a valid source line for, if any)
    DEBUG_stackInfo stack_cur = stack_first;
    while (stack_cur && stack_cur->line<0)
        stack_cur = stack_cur->next;
    if (!stack_cur)
        stack_cur = stack_first;

    _print_stack (g_ide, stack_first, stack_cur);

    _goto_line (g_ide, stack_cur);

    while (TRUE)
    {
        LOG_printf (LOG_DEBUG, "_debug: cmd line loop...\n");
        static char cmdline[256];
        cmdline[0]=0;

        UI_setTextStyle (g_ide->view_console, UI_TEXT_STYLE_KEYWORD);
        IDE_cprintf (g_ide, "dbg (h for help) > ");
        UI_setTextStyle (g_ide->view_console, UI_TEXT_STYLE_TEXT);
        IDE_readline (g_ide, cmdline, 256);

        IDE_cprintf (g_ide, "\n\n");

        // split cmdline and arguments

        char *parts[MAX_NUM_PARTS];
        uint16_t num_parts = 0;

        char *p = cmdline;
        while (*p && (num_parts < MAX_NUM_PARTS))
        {
            // skip whitespace
            while (*p && is_whitespace(*p))
                p++;
            if (!(*p))
                break;

            parts[num_parts++] = p;
            while (*p && !is_whitespace(*p))
                p++;
            if (*p)
            {
                *p = 0;
                p++;
            }
        }

        if (!num_parts)
            continue;

        //IDE_cprintf (g_ide, "cmdline parsing result: %d parts, first part: %s\n\n", num_parts, parts[0]);
        char *cmd = parts[0];

        if (cmd[0] == 'd')                          // disassemble
        {
            if (stack_cur)
            {
                IDE_cprintf (g_ide, "disassembly:\n\n");
                DEBUG_disasm (g_ide, stack_cur->pc, stack_cur->pc+32);
                IDE_cprintf (g_ide, "\n");
            }
            continue;
        }

        if (cmd[0] == 'e')                          // exit/terminate
        {
            // manipulate the return PC, make it point to the exit function:
            //IDE_cprintf (g_ide, "----> setting PC to 0x%08lx\n", g_dbgEnv.u.dbg.msg.debug_exitFn);
            g_dbgPC = g_dbgEnv.u.dbg.msg.debug_exitFn;
            g_dbgSR &= 0x7FFF;
            ReplyMsg (&msg->msg);
            break;
        }

        if (cmd[0] == 'c')                          // continue
        {
            LOG_printf (LOG_DEBUG, "_debug: continue...\n");
            g_dbgEnv.state = DEBUG_stateRunning;
            g_dbgSR &= 0x7FFF;
            ReplyMsg (&msg->msg);
            LOG_printf (LOG_DEBUG, "_debug: continue... done.\n");
            break;
        }

        if (cmd[0] == 'm')                          // mem dump (FIXME: argument)
        {
            IDE_cprintf (g_ide, "PC mem dump:\n\n");
            _hexdump (g_ide, (UBYTE *)g_dbgPC, 32, 16);
            IDE_cprintf (g_ide, "\n");
            continue;
        }

        if (cmd[0] == 's')                          // step
        {
            LOG_printf (LOG_DEBUG, "_debug: step...\n");
            g_dbgEnv.state = DEBUG_stateRunning;
            g_dbgSR |= 0x8000;
            ReplyMsg (&msg->msg);
            LOG_printf (LOG_DEBUG, "_debug: step... done.\n");
            break;
        }

        if ((cmd[0]=='U') || (cmd[0]=='D'))         // stack up/down
        {
            if (cmd[0]=='U')
            {
                if (stack_cur && stack_cur->prev)
                    stack_cur = stack_cur->prev;
            }
            else
            {
                if (stack_cur && stack_cur->next)
                    stack_cur = stack_cur->next;
            }
            _print_stack (g_ide, stack_first, stack_cur);
            _goto_line (g_ide, stack_cur);
            continue;
        }

        if (cmd[0]=='v')                            // variables
        {
            _print_variables (g_ide, stack_cur);
            continue;
        }

        if (cmd[0]=='w')                            // where
        {
            _print_stack (g_ide, stack_first, stack_cur);
            continue;
        }

        if (cmd[0]=='r')                            // registers
        {
            IDE_cprintf (g_ide, "register dump:\n\n");

            IDE_cprintf (g_ide, "d0=%08lx d1=%08lx d2=%08lx d3=%08lx\n", g_dbgStateBuf.d0, g_dbgStateBuf.d1, g_dbgStateBuf.d2, g_dbgStateBuf.d3);
            IDE_cprintf (g_ide, "d4=%08lx d5=%08lx d6=%08lx d7=%08lx\n", g_dbgStateBuf.d4, g_dbgStateBuf.d5, g_dbgStateBuf.d6, g_dbgStateBuf.d7);
            IDE_cprintf (g_ide, "a0=%08lx a1=%08lx a2=%08lx a3=%08lx\n", g_dbgStateBuf.a0, g_dbgStateBuf.a1, g_dbgStateBuf.a2, g_dbgStateBuf.a3);
            IDE_cprintf (g_ide, "a4=%08lx a5=%08lx a6=%08lx a7=%08lx\n", g_dbgStateBuf.a4, g_dbgStateBuf.a5, g_dbgStateBuf.a6, g_dbgStateBuf.a7);

            IDE_cprintf (g_ide, "\nSR=%04x PC=%08lx FMT=%04x\n\n", g_dbgSR, g_dbgPC, g_dbgFMT);
            continue;
        }

        // print help

        IDE_cprintf (g_ide, "available commands:\n\n");

        IDE_cprintf (g_ide, "c        - continue\n");
        IDE_cprintf (g_ide, "d        - disassemble\n");
        IDE_cprintf (g_ide, "e        - exit (terminate program)\n");
        IDE_cprintf (g_ide, "h        - this help text\n");
        IDE_cprintf (g_ide, "m <addr> - memory dump\n");
        IDE_cprintf (g_ide, "r        - register dump\n");
        IDE_cprintf (g_ide, "s        - step\n");
        IDE_cprintf (g_ide, "U/D      - stack move up/down\n");
        IDE_cprintf (g_ide, "v        - variables\n");
        IDE_cprintf (g_ide, "w        - where (stack trace)\n");
        IDE_cprintf (g_ide, "\n");
    }

    LOG_printf (LOG_DEBUG, "_debug: IDE_clearHilight...\n");

    IDE_clearHilight(g_ide);

    LOG_printf (LOG_DEBUG, "_debug: done...\n");
}

uint16_t DEBUG_handleMessages(void)
{
	//LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: start, g_debugPort=0x%08lx\n", g_debugPort);
    USHORT key = KEY_NONE;
    while (TRUE)
    {
        LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: GetMsg...\n");
        struct DebugMsg *m = (struct DebugMsg *) GetMsg(g_debugPort);
        LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: GetMsg returned: 0x%08lx\n", (ULONG)m);
        if (!m)
            return key;

        // is this our dbg process or the help window ?

        if (m->port == &g_helpEnv.childProc->pr_MsgPort)
        {
            struct WBStartup *msg = (struct WBStartup *) m;
            if (msg->sm_Message.mn_Node.ln_Type == NT_REPLYMSG)
            {
                LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: this is a wb startup reply message for our help window -> state is STOPPED now.\n");
                g_helpEnv.state = DEBUG_stateStopped;
                if (g_helpEnv.seglist)
                {
                    UnLoadSeg (g_helpEnv.seglist);
                    g_helpEnv.seglist = 0l;
                }
                if (g_helpEnv.u.wb.arg0.wa_Lock)
                {
                    UnLock (g_helpEnv.u.wb.arg0.wa_Lock);
                    g_helpEnv.u.wb.arg0.wa_Lock = 0l;
                }
                if (g_helpEnv.childHomeDirLock)
                {
                    UnLock (g_helpEnv.childHomeDirLock);
                    g_helpEnv.childHomeDirLock = 0l;
                }
            }
        }
        else
        {
            struct DebugMsg *msg = (struct DebugMsg *) m;
            if (msg->debug_sig == DEBUG_SIG)
            {
                LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: DEBUG message detected, cmd=%d, succ=0x%08lx\n", msg->debug_cmd, msg->msg.mn_Node.ln_Succ);
                switch (msg->debug_cmd)
                {
                    case DEBUG_CMD_START:
                        if ((m->port == &g_dbgEnv.childProc->pr_MsgPort) && (msg->msg.mn_Node.ln_Type == NT_REPLYMSG))
                        {
                            LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: this is a debug reply message for debug child -> state is STOPPED now.\n");
                            //U_delay(2000);
                            g_dbgEnv.state = DEBUG_stateStopped;
                            LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: pool reset\n");
                            //U_delay(2000);
                            U_poolReset (UP_runChild);

                            //printf ("program stopped, ERR is %ld\n", msg->code);
                            LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: program stopped, ERR is %ld, trap code is %ld\n", msg->u.err, g_trapCode);
                            //U_delay(2000);
                            g_dbgEnv.u.dbg.err = msg->u.err;
                            key = KEY_STOPPED;
                            LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: program stopped -> done.\n");
                            //U_delay(2000);
                        }
                        else
                        {
                            LOG_printf (LOG_ERROR, "DEBUG_handleMessages: invalid DEBUG_CMD_START command received\n");
                        }
                        break;
                    case DEBUG_CMD_PUTC:
                        //LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: DEBUG_CMD_PUTC c=%d\n", msg->u.c);
                        IDE_cprintf (g_ide, "%c", msg->u.c);
                        ReplyMsg (&msg->msg);
                        break;
                    case DEBUG_CMD_PUTS:
                        //LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: DEBUG_CMD_PUTS str=\"%s\" (0x%08lx)\n", msg->u.str, msg->u.str);
                        IDE_cprintf (g_ide, "%s", msg->u.str);
                        ReplyMsg (&msg->msg);
                        break;
                    case DEBUG_CMD_TRAP:
                        LOG_printf (LOG_DEBUG, "DEBUG_handleMessages: DEBUG_CMD_TRAP g_terminate=%d\n", g_terminate);
                        if (g_terminate)
                        {
                            g_dbgPC = g_dbgEnv.u.dbg.msg.debug_exitFn;
                            ReplyMsg (&msg->msg);
                        }
                        else
                        {
                            g_dbgEnv.state = DEBUG_stateTrapped;
                            g_trapMsg = &msg->msg;
                            _debug (msg);
                        }
                        break;
                }
            }
        }
    }
}

#if 0
ULONG DEBUG_getERRCode(void)
{

    return g_dbgEnv.u.dbg.err;
}
#endif

#if 0
void DEBUG_freeze (void)
{
	LOG_printf (LOG_INFO, "DEBUG_stop: Freeze...\n");

	BOOL done = FALSE;
	while (!done)
	{
		Forbid();

        if (g_childProc->pr_Task.tc_State == TS_READY)
        {
            Remove ((struct Node *) g_childProc);
            g_childProc->pr_Task.tc_State = (BYTE) TS_FROZEN;
            Enqueue ((struct List *) &SysBase->TaskWait, (struct Node *) g_childProc);
        }
        else
        {
            LOG_printf (LOG_INFO, "DEBUG_stop: not TS_READY!\n");
            Permit();
			Delay(1);
            continue;
        }

        Permit();

		ULONG *sp = (ULONG*) g_childProc->pr_Task.tc_SPReg;
		ULONG *spp = sp+1;
		LOG_printf (LOG_INFO, "DEBUG_stop: sp=0x%08lx *sp=0x%08lx spp=0x%08lx *spp=0x%08lx exitfn=0x%08lx\n", (ULONG)sp, *sp, spp, *spp, g_dbgMsg.exitFn);

		ULONG rts = *spp;
		if ((rts & 0xfff00000) != 0x00f00000)
		{
			*spp = (ULONG) g_dbgMsg.exitFn;
			done = TRUE;
			LOG_printf (LOG_INFO, "DEBUG_stop: force exit!\n");
		}

		Forbid();
		Remove ((struct Node *) g_childProc);
		g_childProc->pr_Task.tc_State = (BYTE) TS_READY;
		Enqueue ((struct List *) &SysBase->TaskReady, (struct Node *) g_childProc);
		Permit();

		if (!done)
		{
			Delay(1);
		}
	}

	LOG_printf (LOG_INFO, "DEBUG_stop: done\n");
}
#endif

void DEBUG_break (bool waitForTermination)
{
    switch (g_dbgEnv.state)
    {
        case DEBUG_stateStopped:
            return;

        case DEBUG_stateRunning:
            LOG_printf (LOG_INFO, "DEBUG_break: sending CTRL+C signal to child\n");
            Signal (&g_dbgEnv.childProc->pr_Task, SIGBREAKF_CTRL_C);
            break;

        case DEBUG_stateTrapped:
            g_dbgPC = g_dbgEnv.u.dbg.msg.debug_exitFn;
            ReplyMsg (g_trapMsg);
            break;

    }

    if (waitForTermination)
        UI_waitDebugTerm();
}

void DEBUG_init (IDE_instance ide, struct MsgPort *debugPort)
{
    g_ide         = ide;
	g_parentTask  = FindTask(NULL);
    //g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
    g_debugPort   = debugPort;

    has_fpu = SysBase->AttnFlags & (AFF_68881 | AFF_68882);
    has_68060_or_up = (SysBase->AttnFlags & AFF_68060);
    has_68040_or_up = has_68060_or_up || (SysBase->AttnFlags & AFF_68040);
    has_68030_or_up = has_68040_or_up || (SysBase->AttnFlags & AFF_68030);
    has_68020_or_up = has_68030_or_up || (SysBase->AttnFlags & AFF_68020);
    has_68010_or_up = has_68020_or_up || (SysBase->AttnFlags & AFF_68010);
}

DEBUG_state DEBUG_getState(void)
{
    return g_dbgEnv.state;
}


#else

// FIXME: implement?

DEBUG_state DEBUG_getState(void)
{
    return DEBUG_stateStopped;
}

#endif
