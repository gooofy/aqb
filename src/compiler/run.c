#include <assert.h>
#include <stdlib.h>

#include "run.h"
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

typedef struct RUN_env_ *RUN_env;

struct RUN_env_
{
    RUN_state          state;
    struct Process    *childProc;
    char              *binfn;
    BPTR               childHomeDirLock;
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

static struct MsgPort    *g_debugPort;
static struct Task       *g_parentTask;
static struct Task       *g_childTask;
static struct RUN_env_    g_dbgEnv   = {RUN_stateStopped, NULL, NULL, 0, 0};
static struct RUN_env_    g_helpEnv  = {RUN_stateStopped, NULL, NULL, 0, 0};
static ULONG              g_trapCode = 0;
static UWORD              g_dbgSR    = 0;
static ULONG              g_dbgPC    = 0;
static UWORD              g_dbgFMT   = 0;
static struct dbgState    g_dbgStateBuf;

static BOOL has_fpu         = FALSE;
static BOOL has_68060_or_up = FALSE;
static BOOL has_68040_or_up = FALSE;
static BOOL has_68030_or_up = FALSE;
static BOOL has_68020_or_up = FALSE;
static BOOL has_68010_or_up = FALSE;

// Usage:
//     hexdump(addr, len, perLine);
//         addr:    the address to start dumping from.
//         len:     the number of bytes to dump.
//         perLine: number of bytes on each output line.

void hexdump ( const void * addr, const int len, int perLine) {

    int i;
    unsigned char buff[perLine+1];
    const unsigned char * pc = (const unsigned char *)addr;

    for (i = 0; i < len; i++) {
        if ((i % perLine) == 0) {
            if (i != 0) UI_tprintf ("  %s\n", buff);
            UI_tprintf ("  %04x ", i);
        }

        UI_tprintf (" %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % perLine] = '.';
        else
            buff[i % perLine] = pc[i];
        buff[(i % perLine) + 1] = '\0';
    }

    while ((i % perLine) != 0) {
        UI_tprintf ("   ");
        i++;
    }

    UI_tprintf ("  %s\n", buff);
}

extern APTR _unfreeze;

asm (
"__unfreeze:\n"

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
);


void _freeze_myself(void)
{
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

	Supervisor((void *)&_unfreeze);
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
"__trap_handler_00:\n"
    // FIXME
"       move.l  ssp@+, _g_trapCode;\n" 	    // save trap code
"       "
"       rte;"

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
"       move.w  a5@(-6), ssp@-;\n"          // sr

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

#if 0
static LI_segmentList _loadSeg(char *binfn)
{
    LOG_printf (LOG_INFO, "Loading %s ...\n", binfn);

    FILE *f = fopen (binfn, "r");
    if (!f)
    {
        LOG_printf (LOG_ERROR, "*** ERROR: failed to open %s\n\n", binfn);
        return NULL;
    }
    LI_segmentList sl = LI_SegmentList();

    if (!LI_segmentListReadLoadFile (sl, binfn, f))
    {
        fclose (f);
        LOG_printf (LOG_ERROR, "*** ERROR: failed to read %s\n\n", binfn);
        return NULL;
    }
    fclose (f);

    return sl;
}
#endif

static void _launch_process (RUN_env env, char *binfn, char *arg1, bool dbg)
{
    env->binfn = binfn;

#if 0
    // FIXME: experimental custom loader, handles debug info
    LI_segmentList sl = _loadSeg(binfn);
    if (!sl)
        return;
#endif

#if 1

    LOG_printf (LOG_DEBUG, "RUN _launch_process: loading %s ...\n\n", binfn);
    env->seglist = LoadSeg((STRPTR)binfn);
    if (!env->seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", binfn);
        return;
    }

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

    env->state = RUN_stateRunning;

	LOG_printf (LOG_DEBUG, "RUN _launch_process: done. state is %d now.\n", env->state);
    #endif
}

void RUN_start (const char *binfn)
{
    _launch_process (&g_dbgEnv, (char *)binfn, /*arg=*/NULL, /*dbg=*/TRUE);
}

void RUN_help (char *binfn, char *arg1)
{
    _launch_process (&g_dbgEnv, (char *)binfn, /*arg=*/arg1, /*dbg=*/FALSE);
}

static void _debug(struct DebugMsg *msg)
{
    UI_tprintf ("\n\n*** AQB DEBUG ***\n\n");

    switch (g_trapCode)
    {
        case 2:
            UI_tprintf("ACCESS FAULT\n\n");
            break;
        case 3:
            UI_tprintf("ADDRESS ERROR\n\n");
            break;
        case 4:
            UI_tprintf("ILLEGAL INSTRUCTION\n\n");
            break;
        case 5:
            UI_tprintf("INTEGER DIVIDE BY ZERO\n\n");
            break;
        case 32:
            UI_tprintf("CTRL-C BREAK\n\n");
            break;
        case 33:
            UI_tprintf("BREAKPOINT HIT\n\n");
            break;
        default:
            UI_tprintf("TRAP #%d (\?\?\?) occured.\n\n", g_trapCode);
    }

    // register dump

    UI_tprintf ("d0=%08lx d1=%08lx d2=%08lx d3=%08lx\n", g_dbgStateBuf.d0, g_dbgStateBuf.d1, g_dbgStateBuf.d2, g_dbgStateBuf.d3);
    UI_tprintf ("d4=%08lx d5=%08lx d6=%08lx d7=%08lx\n", g_dbgStateBuf.d4, g_dbgStateBuf.d5, g_dbgStateBuf.d6, g_dbgStateBuf.d7);
    UI_tprintf ("a0=%08lx a1=%08lx a2=%08lx a3=%08lx\n", g_dbgStateBuf.a0, g_dbgStateBuf.a1, g_dbgStateBuf.a2, g_dbgStateBuf.a3);
    UI_tprintf ("a4=%08lx a5=%08lx a6=%08lx a7=%08lx\n", g_dbgStateBuf.a4, g_dbgStateBuf.a5, g_dbgStateBuf.a6, g_dbgStateBuf.a7);

    UI_tprintf ("\nSR=%04x PC=%08lx FMT=%04x\n", g_dbgSR, g_dbgPC, g_dbgFMT);
    UI_tprintf ("\nPC mem dump:\n");
    hexdump ((UBYTE *)g_dbgPC, 32, 16);

    BOOL finished = FALSE;
    while (!finished)
    {
        UI_tprintf ("\n\nPRESS C:continue, E:exit\n\n");
        uint16_t key = UI_waitkey();
        switch (key)
        {
            case 'c':
            case 'C':
                ReplyMsg (&msg->msg);
                finished = TRUE;
                break;
            case 'e':
            case 'E':
                // manipulate the return PC, make it point to the exit function:
                //UI_tprintf ("----> setting PC to 0x%08lx\n", g_dbgEnv.u.dbg.msg.debug_exitFn);
                g_dbgPC = g_dbgEnv.u.dbg.msg.debug_exitFn;
                ReplyMsg (&msg->msg);
                finished = TRUE;
                break;

            default:
                UI_tprintf ("\n\n*** ERROR: UNKNOWN KEY ***\n\n");
        }
    }
}

uint16_t RUN_handleMessages(void)
{
	LOG_printf (LOG_DEBUG, "RUN_handleMessages: start, g_debugPort=0x%08lx\n", g_debugPort);
    USHORT key = KEY_NONE;
    while (TRUE)
    {
        LOG_printf (LOG_DEBUG, "RUN_handleMessages: GetMsg...\n");
        struct DebugMsg *m = (struct DebugMsg *) GetMsg(g_debugPort);
        LOG_printf (LOG_DEBUG, "RUN_handleMessages: GetMsg returned: 0x%08lx\n", (ULONG)m);
        if (!m)
            return key;

        // is this our dbg process or the help window ?

        if (m->port == &g_helpEnv.childProc->pr_MsgPort)
        {
            struct WBStartup *msg = (struct WBStartup *) m;
            if (msg->sm_Message.mn_Node.ln_Type == NT_REPLYMSG)
            {
                LOG_printf (LOG_DEBUG, "RUN_handleMessages: this is a wb startup reply message for our help window -> state is STOPPED now.\n");
                g_helpEnv.state = RUN_stateStopped;
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
                LOG_printf (LOG_DEBUG, "RUN_handleMessages: DEBUG message detected, cmd=%d, succ=0x%08lx\n", msg->debug_cmd, msg->msg.mn_Node.ln_Succ);
                switch (msg->debug_cmd)
                {
                    case DEBUG_CMD_START:
                        if ((m->port == &g_dbgEnv.childProc->pr_MsgPort) && (msg->msg.mn_Node.ln_Type == NT_REPLYMSG))
                        {
                            LOG_printf (LOG_DEBUG, "RUN_handleMessages: this is a debug reply message for our help window -> state is STOPPED now.\n");
                            g_dbgEnv.state = RUN_stateStopped;
                            if (g_dbgEnv.seglist)
                            {
                                UnLoadSeg (g_dbgEnv.seglist);
                                g_dbgEnv.seglist = 0l;
                            }

                            //printf ("program stopped, ERR is %ld\n", msg->code);
                            LOG_printf (LOG_DEBUG, "RUN_handleMessages: program stopped, ERR is %ld, trap code is %ld\n", msg->u.err, g_trapCode);
                            g_dbgEnv.u.dbg.err = msg->u.err;
                            key = KEY_STOPPED;
                        }
                        else
                        {
                            LOG_printf (LOG_ERROR, "RUN_handleMessages: invalid DEBUG_CMD_START command received\n");
                        }
                        break;
                    case DEBUG_CMD_PUTC:
                        LOG_printf (LOG_DEBUG, "RUN_handleMessages: DEBUG_CMD_PUTC c=%d\n", msg->u.c);
                        UI_tprintf ("%c", msg->u.c);
                        ReplyMsg (&msg->msg);
                        break;
                    case DEBUG_CMD_PUTS:
                        LOG_printf (LOG_DEBUG, "RUN_handleMessages: DEBUG_CMD_PUTS str=\"%s\" (0x%08lx)\n", msg->u.str, msg->u.str);
                        UI_tprintf ("%s", msg->u.str);
                        ReplyMsg (&msg->msg);
                        break;
                    case DEBUG_CMD_TRAP:
                        _debug (msg);
                        break;
                }
            }
        }
    }
}

#if 0
ULONG RUN_getERRCode(void)
{

    return g_dbgEnv.u.dbg.err;
}
#endif

#if 0
void RUN_freeze (void)
{
	LOG_printf (LOG_INFO, "RUN_stop: Freeze...\n");

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
            LOG_printf (LOG_INFO, "RUN_stop: not TS_READY!\n");
            Permit();
			Delay(1);
            continue;
        }

        Permit();

		ULONG *sp = (ULONG*) g_childProc->pr_Task.tc_SPReg;
		ULONG *spp = sp+1;
		LOG_printf (LOG_INFO, "RUN_stop: sp=0x%08lx *sp=0x%08lx spp=0x%08lx *spp=0x%08lx exitfn=0x%08lx\n", (ULONG)sp, *sp, spp, *spp, g_dbgMsg.exitFn);

		ULONG rts = *spp;
		if ((rts & 0xfff00000) != 0x00f00000)
		{
			*spp = (ULONG) g_dbgMsg.exitFn;
			done = TRUE;
			LOG_printf (LOG_INFO, "RUN_stop: force exit!\n");
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

	LOG_printf (LOG_INFO, "RUN_stop: done\n");
}
#endif

void RUN_break (void)
{
	LOG_printf (LOG_INFO, "RUN_break: sending CTRL+C signal to child\n");

    Signal (&g_dbgEnv.childProc->pr_Task, SIGBREAKF_CTRL_C);
}

void RUN_init (struct MsgPort *debugPort)
{
	g_parentTask = FindTask(NULL);
    //g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
    g_debugPort   = debugPort;

    has_fpu = SysBase->AttnFlags & (AFF_68881 | AFF_68882);
    has_68060_or_up = (SysBase->AttnFlags & AFF_68060);
    has_68040_or_up = has_68060_or_up || (SysBase->AttnFlags & AFF_68040);
    has_68030_or_up = has_68040_or_up || (SysBase->AttnFlags & AFF_68030);
    has_68020_or_up = has_68030_or_up || (SysBase->AttnFlags & AFF_68020);
    has_68010_or_up = has_68020_or_up || (SysBase->AttnFlags & AFF_68010);
}

RUN_state RUN_getState(void)
{
    return g_dbgEnv.state;
}

void  RUN_handleEvent (uint16_t key)
{
#if 0
    BOOL bInRefresh = FALSE;
    switch (key)
    {
        case KEY_CTRL_C:
        case KEY_CTRL_Q:
        case KEY_QUIT:
            IDE_exit(ed);
            break;

        case KEY_REFRESH:
            UI_beginRefresh();
            bInRefresh = TRUE;
            invalidateAll (ed);
            break;

        case KEY_HELP:
        case KEY_F1:
            show_help(ed);
            break;

		case KEY_COLORSCHEME_0:
		case KEY_COLORSCHEME_1:
		case KEY_COLORSCHEME_2:
		case KEY_COLORSCHEME_3:
		case KEY_COLORSCHEME_4:
		case KEY_COLORSCHEME_5:
			UI_setColorScheme (key - KEY_COLORSCHEME_0);
            invalidateAll (ed);
			break;

		case KEY_FONT_0:
		case KEY_FONT_1:
			UI_setFont (key - KEY_FONT_0);
            initWindowSize (ed);
            invalidateAll (ed);
			break;

        case KEY_NONE:
            break;

        default:
            UI_bell();
            break;

    }

    scroll(ed);
    repaint(ed);
    if (bInRefresh)
        UI_endRefresh();
#endif

}

#else

// FIXME: implement?

RUN_state RUN_getState(void)
{
    return RUN_stateStopped;
}

void  RUN_handleEvent (uint16_t key)
{
    // FIXME
}

#endif
