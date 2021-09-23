#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"

static RUN_state        g_debugState = RUN_stateStopped;

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

static struct MsgPort  *g_debugPort;

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

#define TS_FROZEN          0xff

#define DEBUG_SIG 0xDECA11ED

/* we send this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
	struct MsgPort *port;
    ULONG           debug_sig;
	APTR            exitFn;
};

static struct Task       *g_parentTask;
static struct Process    *g_childProc;
static char              *g_binfn;
static BPTR               g_currentDir;
static BPTR               g_seglist;
static struct DebugMsg    g_dbgMsg;


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

    LOG_printf (LOG_DEBUG, "RUN_start: loading %s ...\n\n", g_binfn);
    g_seglist = LoadSeg((STRPTR)g_binfn);
    if (!g_seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", g_binfn);
        return;
    }

    LOG_printf (LOG_DEBUG, "RUN_start: CreateNewProc for %s ...\n", binfn);
    g_childProc = CreateNewProcTags(NP_Seglist,     (ULONG) g_seglist,
                                    NP_CloseOutput, FALSE,
                                    NP_StackSize,   DEFAULT_STACKSIZE,
								    NP_Name,        (ULONG) g_binfn,
                                    NP_Output,      Output());


    LOG_printf (LOG_DEBUG, "RUN_start: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) g_childProc);

    // send startup message

	g_dbgMsg.msg.mn_Node.ln_Succ = NULL;
	g_dbgMsg.msg.mn_Node.ln_Pred = NULL;
	g_dbgMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
	g_dbgMsg.msg.mn_Node.ln_Pri  = 0;
	g_dbgMsg.msg.mn_Node.ln_Name = NULL;
    g_dbgMsg.msg.mn_ReplyPort    = g_debugPort;
	g_dbgMsg.msg.mn_Length       = sizeof(struct DebugMsg);
	g_dbgMsg.port                = &g_childProc->pr_MsgPort;
	g_dbgMsg.debug_sig           = DEBUG_SIG;
	g_dbgMsg.exitFn              = NULL;

	LOG_printf (LOG_DEBUG, "RUN_start: Send debug msg...\n");

	PutMsg (&g_childProc->pr_MsgPort, &g_dbgMsg.msg);

    g_debugState = RUN_stateRunning;

	LOG_printf (LOG_DEBUG, "RUN_start: done.\n");
}

void RUN_handleMessages(void)
{
    while (TRUE)
    {
        struct DebugMsg *msg = (struct DebugMsg *) GetMsg(g_debugPort);
        if (!msg)
            return;
        if (   (msg->msg.mn_Node.ln_Type == NT_REPLYMSG)
            && (msg->debug_sig == DEBUG_SIG))
            g_debugState = RUN_stateStopped;
    }
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

void RUN_init (struct MsgPort *debugPort)
{
	g_parentTask = FindTask(NULL);
    g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
    g_debugPort  = debugPort;
}

#endif

RUN_state RUN_getState(void)
{
    return g_debugState;
}

