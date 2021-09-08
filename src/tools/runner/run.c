#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"

#ifdef __amigaos__

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <inline/exec.h>
#include <inline/dos.h>
extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#define DEFAULT_PRI           0
#define DEFAULT_STACKSIZE 32768

struct FakeSegList
{
	ULONG 	length;
	ULONG 	next;
	UWORD 	jump;
	APTR 	code;
};

static struct Task *g_parentTask;

static int                g_termSignal;
static struct FileHandle *g_output;
static char              *g_binfn;
static BPTR               g_currentDir;

typedef LONG (*startup_t) ( register STRPTR cmdline __asm("a0"), register ULONG cmdlen __asm("d0") );

static void runner (void)
{
	struct Process *me = (struct Process *) FindTask(NULL);
    me->pr_CurrentDir = g_currentDir;

    LOG_printf (LOG_INFO, "loading %s ...\n\n", g_binfn);
    BPTR seglist = LoadSeg((STRPTR)g_binfn);

    if (!seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", g_binfn);
        Signal (g_parentTask, 1<<g_termSignal);
        return;
    }

    LOG_printf (LOG_INFO, "running %s ...\n\n", g_binfn);

    me->pr_COS = MKBADDR(g_output);

    ULONG *code = (ULONG *) BADDR(seglist);
    code++;
    startup_t f = (startup_t) code;

    //printf ("run: before f...\n");

    f((STRPTR)"fake_aqb_env", 12);

    //printf ("run: after f... 1\n");
    UnLoadSeg(seglist);

    //printf ("run: after f... 2\n");
	LOG_printf (LOG_DEBUG, "runner ends, sending signal\n");

    //printf ("run: after f... 3\n");
	Signal (g_parentTask, 1<<g_termSignal);
    //printf ("run: after f... 4\n");
}


void RUN_start (const char *binfn)
{
	struct FakeSegList *segl;

	if ( (segl = AllocMem (sizeof(struct FakeSegList),MEMF_CLEAR|MEMF_PUBLIC)) )
	{
		segl->length = 16;
		segl->jump   = 0x4EF9;
		segl->code   = runner;

        g_binfn = (char *)binfn;

		// LOG_printf (LOG_INFO, "running %s ...\n\n", binfn);
		struct MsgPort *msgport = CreateProc((STRPTR) binfn, DEFAULT_PRI, MKBADDR(segl), DEFAULT_STACKSIZE);
		assert(msgport);
	}
	else
	{
		LOG_printf (LOG_ERROR, "run: failed to allocate memory for fake seglist!\n");
	}

}

void RUN_init (int termSignal, struct FileHandle *output)
{
    g_termSignal = termSignal;
    g_output     = output;
	g_parentTask = FindTask(NULL);
    g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
}

#endif
