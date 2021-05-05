#include <assert.h>
#include <stdlib.h>

#include "run.h"
#include "logger.h"
#include "util.h"
#include "terminal.h"

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

static char *g_binfn;
static BPTR  g_currentDir;

//typedef int (*startup_t)(void);

//long add( register long a __asm("d0"), register long b __asm("d1") );
typedef LONG (*startup_t) ( register STRPTR cmdline __asm("a0"), register ULONG cmdlen __asm("d0") );

static void runner (void)
{
    LOG_printf (LOG_INFO, "loading %s ...\n\n", g_binfn);
    //BPTR seglist = LoadSeg((STRPTR)g_binfn);
    BPTR seglist = LoadSeg((STRPTR)"SYS:x/foo");

    if (!seglist)
    {
        LOG_printf (LOG_ERROR, "failed to load %s\n\n", g_binfn);
        Signal (g_parentTask, 1<<TE_termSignal());
        return;
    }

    LOG_printf (LOG_INFO, "running %s ...\n\n", g_binfn);

	struct Process *me = (struct Process *) FindTask(NULL);
    me->pr_COS = MKBADDR(TE_output());
    me->pr_CurrentDir = g_currentDir;

    ULONG *code = (ULONG *) BADDR(seglist);
    code++;
    startup_t f = (startup_t) code;

    f((STRPTR)"fake_aqb_env", 12);

    //Write (o, "12345\n", 6);
	//Delay(50);

    UnLoadSeg(seglist);

	LOG_printf (LOG_DEBUG, "runner ends, sending signal\n");

	Signal (g_parentTask, 1<<TE_termSignal());
}


void RUN_run (const char *binfn)
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

        TE_runIO();

		// LOG_printf (LOG_INFO, "%s finished.\n\n", binfn);

        // FIXME: cleanup

	}
	else
	{
		LOG_printf (LOG_ERROR, "run: failed to allocate memory for fake seglist!\n");
	}

}

void RUN_init (void)
{
	g_parentTask = FindTask(NULL);
    g_currentDir = ((struct Process *)g_parentTask)->pr_CurrentDir;
}

#else // no amigaos -> posix / vamos?

void RUN_run (const char *binfn)
{
    assert(FALSE); // FIXME
}

void RUN_init (void)
{
}

#endif
