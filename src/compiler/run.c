#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"
#include "ui.h"

// #define SEND_WBSTARTUP_MSG

static RUN_state        g_debugState = RUN_stateStopped;

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

static struct MsgPort  *g_debugPort;

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

#define TS_FROZEN          0xff

#define DEBUG_SIG 0xDECA11ED
static struct Task       *g_parentTask;
static struct Process    *g_childProc;
static char              *g_binfn;
static BPTR               g_currentDir;
static BPTR               g_seglist;

#ifdef SEND_WBSTARTUP_MSG

static struct WBStartup   g_startupMsg;
static struct WBArg       g_startupArg;

#else

/* we send this instead of WBStartup */
struct DebugMsg
{
    struct Message  msg;
	struct MsgPort *port;
    ULONG           debug_sig;
	ULONG           code;
};

static struct DebugMsg    g_dbgMsg;
static ULONG              g_ERRCode;

#endif

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
									NP_FreeSeglist, FALSE,
									NP_Input,       0l,
                                    NP_Output,      Output(),
                                    NP_CloseInput,  FALSE,
                                    NP_CloseOutput, FALSE,
                                    NP_StackSize,   DEFAULT_STACKSIZE,
								    NP_Name,        (ULONG) g_binfn,
									//NP_WindowPtr,   0l,
									//NP_HomeDir,     0l,
									NP_CopyVars,    FALSE,
									TAG_DONE);

    LOG_printf (LOG_DEBUG, "RUN_start: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) g_childProc);

    // send startup message

#ifdef SEND_WBSTARTUP_MSG

	g_startupMsg.sm_Message.mn_Node.ln_Succ = NULL;
	g_startupMsg.sm_Message.mn_Node.ln_Pred = NULL;
	g_startupMsg.sm_Message.mn_Node.ln_Type = NT_MESSAGE;
	g_startupMsg.sm_Message.mn_Node.ln_Pri  = 0;
	g_startupMsg.sm_Message.mn_Node.ln_Name = NULL;
    g_startupMsg.sm_Message.mn_ReplyPort    = g_debugPort;
	g_startupMsg.sm_Message.mn_Length       = sizeof(struct WBStartup);
	g_startupMsg.sm_Process                 = &g_childProc->pr_MsgPort;
	g_startupMsg.sm_Segment                 = g_seglist;
    g_startupMsg.sm_NumArgs                 = 1;
    g_startupMsg.sm_ToolWindow              = NULL;
    g_startupMsg.sm_ArgList                 = &g_startupArg;

    static char dirbuf[256];
    strncpy (dirbuf, g_binfn, 256);
    *(PathPart((STRPTR)dirbuf)) = 0;

    g_startupArg.wa_Lock = Lock ((STRPTR)dirbuf, ACCESS_READ);
    g_startupArg.wa_Name = (BYTE*) FilePart ((STRPTR)g_binfn);

	LOG_printf (LOG_DEBUG, "RUN_start: Send WBSTartup msg (dirbuf=%s)...\n", dirbuf);

	PutMsg (&g_childProc->pr_MsgPort, &g_startupMsg.sm_Message);

#else

	g_dbgMsg.msg.mn_Node.ln_Succ = NULL;
	g_dbgMsg.msg.mn_Node.ln_Pred = NULL;
	g_dbgMsg.msg.mn_Node.ln_Type = NT_MESSAGE;
	g_dbgMsg.msg.mn_Node.ln_Pri  = 0;
	g_dbgMsg.msg.mn_Node.ln_Name = NULL;
    g_dbgMsg.msg.mn_ReplyPort    = g_debugPort;
	g_dbgMsg.msg.mn_Length       = sizeof(struct DebugMsg);
	g_dbgMsg.port                = &g_childProc->pr_MsgPort;
	g_dbgMsg.debug_sig           = DEBUG_SIG;
	g_dbgMsg.code                = 0;
    g_ERRCode                    = 0;

	LOG_printf (LOG_DEBUG, "RUN_start: Send debug msg...\n");

	PutMsg (&g_childProc->pr_MsgPort, &g_dbgMsg.msg);

#endif

    g_debugState = RUN_stateRunning;

	LOG_printf (LOG_DEBUG, "RUN_start: done.\n");
}

uint16_t RUN_handleMessages(void)
{
	LOG_printf (LOG_DEBUG, "RUN_handleMessages: start...\n");
    USHORT key = KEY_NONE;
    while (TRUE)
    {
#ifdef SEND_WBSTARTUP_MSG
        struct WBStartup *msg = (struct WBStartup *) GetMsg(g_debugPort);
        LOG_printf (LOG_DEBUG, "RUN_handleMessages: GetMsg returned: 0x%08lx\n", (ULONG)msg);
        if (!msg)
            return key;
        if (msg->sm_Message.mn_Node.ln_Type == NT_REPLYMSG)
        {
            LOG_printf (LOG_DEBUG, "RUN_handleMessages: this is a reply message -> state is STOPPED now.\n");
            g_debugState = RUN_stateStopped;
            if (g_seglist)
            {
                UnLoadSeg (g_seglist);
                g_seglist = 0l;
            }
            if (g_startupArg.wa_Lock)
            {
                UnLock (g_startupArg.wa_Lock);
                g_startupArg.wa_Lock = 0l;
            }
        }
#else
        struct DebugMsg *msg = (struct DebugMsg *) GetMsg(g_debugPort);
        if (!msg)
            return key;
        if (   (msg->msg.mn_Node.ln_Type == NT_REPLYMSG)
            && (msg->debug_sig == DEBUG_SIG))
        {
            g_debugState = RUN_stateStopped;
            if (g_seglist)
            {
                UnLoadSeg (g_seglist);
                g_seglist = 0l;
            }

            printf ("program stopped, ERR is %ld\n", msg->code);
            g_ERRCode = msg->code;
            key = KEY_STOPPED;
        }
#endif
    }
}

ULONG RUN_getERRCode(void)
{
    return g_ERRCode;
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

