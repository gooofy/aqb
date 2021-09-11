#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"

#ifdef __amigaos__

#include "amigasupport.h"

#include <exec/execbase.h>
#include <dos/dostags.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

#define TS_FROZEN          0xff

#if 0
struct FakeSegList
{
	ULONG 	length;
	ULONG 	next;
	UWORD 	jump;
	APTR 	code;
};
#endif

/* we send this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
	struct MsgPort *port;
    ULONG           debug_sig;	// 0xDECA11ED
	APTR            exitFn;
};

static struct Task       *g_parentTask;
static struct Process    *g_childProc;
static int                g_termSignal;
static struct FileHandle *g_output;
static char              *g_binfn;
static BPTR               g_currentDir;
static BPTR               g_seglist;
static struct DebugMsg    g_dbgMsg;
static struct MsgPort    *g_replyPort;


#if 0
typedef LONG (*startup_t) ( register STRPTR cmdline __asm("a0"), register ULONG cmdlen __asm("d0") );

static void runner (void)
{
#if 0
	struct Process *me = (struct Process *) FindTask(NULL);
    g_childTask = (struct Task *) me;
    me->pr_CurrentDir = g_currentDir;

    //LOG_printf (LOG_INFO, "run: runner() running %s ...\n\n", g_binfn);

    //me->pr_COS = MKBADDR(g_output);

    ULONG *code = (ULONG *) BADDR(seglist);
    code++;
    //startup_t f = (startup_t) code;

    //LOG_printf (LOG_INFO, "run: runner() before f ...\n\n");
#endif
    while (TRUE);

#if 0
    //f((STRPTR)"fake_aqb_env", 12);

    //LOG_printf (LOG_INFO, "run: runner() after f... 1\n");
    UnLoadSeg(seglist);

	//LOG_printf (LOG_INFO, "run: runner ends, sending signal\n");

	Signal (g_parentTask, 1<<g_termSignal);
#endif
}
#endif

void RUN_start (const char *binfn)
{
    g_binfn = (char *)binfn;

    LOG_printf (LOG_INFO, "RUN_start: loading %s ...\n\n", g_binfn);
    g_seglist = LoadSeg((STRPTR)g_binfn);
    if (!g_seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", g_binfn);
        Signal (g_parentTask, 1<<g_termSignal);
        return;
    }

    LOG_printf (LOG_INFO, "RUN_start: CreateNewProc for %s ...\n", binfn);
    g_childProc = CreateNewProcTags(NP_Output,      (ULONG) MKBADDR(g_output),
                                    NP_Seglist,     (ULONG) g_seglist,
                                    NP_CloseOutput, FALSE,
                                    NP_StackSize,   DEFAULT_STACKSIZE,
								    NP_Name,        (ULONG) g_binfn);


    LOG_printf (LOG_INFO, "RUN_start: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) g_childProc);

    // send startup message

	g_dbgMsg.msg.mn_Node.ln_Succ = NULL;
	g_dbgMsg.msg.mn_Node.ln_Pred = NULL;
	g_dbgMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
	g_dbgMsg.msg.mn_Node.ln_Pri  = 0;
	g_dbgMsg.msg.mn_Node.ln_Name = NULL;
	g_dbgMsg.msg.mn_Length       = sizeof(struct DebugMsg);
	g_dbgMsg.port                = &g_childProc->pr_MsgPort;
	g_dbgMsg.debug_sig           = 0xDECA11ED;
	g_dbgMsg.exitFn              = NULL;

	LOG_printf (LOG_INFO, "RUN_start: Send debug msg...\n");

	PutMsg (&g_childProc->pr_MsgPort, &g_dbgMsg.msg);

	LOG_printf (LOG_INFO, "RUN_start: done.\n");
}

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

    Signal (&g_childProc->pr_Task, SIGBREAKF_CTRL_C);
}


void RUN_init (int termSignal, struct FileHandle *output)
{
    g_termSignal = termSignal;
    g_output     = output;
	g_parentTask = FindTask(NULL);
    g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
    g_replyPort  = ASUP_create_port ((STRPTR) "AQB debug reply port", 0);
    if (!g_replyPort)
    {
        // FIXME
        fprintf (stderr, "run: failed to create debug reply port!\n");
        exit(42);
    }
}

#endif
