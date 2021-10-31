#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"
#include "ui.h"
#include "options.h"

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

/* we send this instead of WBStartup when running a debug process */
struct DebugMsg
{
    struct Message  msg;
	struct MsgPort *port;
    ULONG           debug_sig;
    UWORD           debug_cmd;
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

static struct MsgPort    *g_debugPort;
static struct Task       *g_parentTask;
static struct RUN_env_    g_dbgEnv   = {RUN_stateStopped, NULL, NULL, 0, 0};
static struct RUN_env_    g_helpEnv  = {RUN_stateStopped, NULL, NULL, 0, 0};

static void _launch_process (RUN_env env, char *binfn, char *arg1, bool dbg)
{
    env->binfn = binfn;

    LOG_printf (LOG_DEBUG, "RUN _launch_process: loading %s ...\n\n", binfn);
    env->seglist = LoadSeg((STRPTR)binfn);
    if (!env->seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", binfn);
        return;
    }

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

    LOG_printf (LOG_DEBUG, "RUN _launch_process: CreateProc for %s ... done. process: 0x%08lx\n", binfn, (ULONG) env->childProc);

    // send startup message

    if (dbg)
    {

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
}

void RUN_start (const char *binfn)
{
    _launch_process (&g_dbgEnv, (char *)binfn, /*arg=*/NULL, /*dbg=*/TRUE);
}

void RUN_help (char *binfn, char *arg1)
{
    _launch_process (&g_dbgEnv, (char *)binfn, /*arg=*/arg1, /*dbg=*/FALSE);
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
                        LOG_printf (LOG_INFO, "%c", msg->u.c);
                        ReplyMsg (&msg->msg);
                        break;
                    case DEBUG_CMD_PUTS:
                        LOG_printf (LOG_DEBUG, "RUN_handleMessages: DEBUG_CMD_PUTS str=\"%s\" (0x%08lx)\n", msg->u.str, msg->u.str);
                        LOG_printf (LOG_INFO, "%s", msg->u.str);
                        ReplyMsg (&msg->msg);
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
    g_debugPort  = debugPort;
    // FIXME: remove g_dbgEnv->childProc->pr_MsgPort = NULL;
    // FIXME: remove g_helpEnv->childProc->pr_MsgPort = NULL;
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
