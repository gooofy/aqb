
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

struct FakeSegList
{
	ULONG 	length;
	ULONG 	next;
	UWORD 	jump;
	APTR 	code;
};

typedef LONG (*startup_t) ( register STRPTR cmdline __asm("a0"), register ULONG cmdlen __asm("d0") );

#define BINFN "SYS:x/foo"

int main (int argc, char *argv[])
{
    printf ("loading %s ...\n\n", BINFN);
    BPTR seglist = LoadSeg((STRPTR)BINFN);

    if (!seglist)
    {
        printf ("failed to load %s\n\n", BINFN);
        return 23;
    }

    printf ("running %s ...\n\n", BINFN);

	//struct Process *me = (struct Process *) FindTask(NULL);
    //me->pr_COS = MKBADDR(TE_output());
    //me->pr_CurrentDir = g_currentDir;

    ULONG *code = (ULONG *) BADDR(seglist);
    code++;
    startup_t f = (startup_t) code;

    f((STRPTR)"fake_aqb_env", 12);

	printf ("returned from sub, UnLoadSeg...\n");
    UnLoadSeg(seglist);

	printf ("runner ends\n");

    return 0;
}

