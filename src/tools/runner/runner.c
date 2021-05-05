
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>

#ifdef __amigaos__
#include <exec/types.h>
#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include <inline/exec.h>
#include <inline/dos.h>

extern struct ExecBase      *SysBase;
extern struct DOSBase       *DOSBase;

#endif

#ifdef __amigaos__

#define MIN_STACKSIZE 64*1024

static void check_stacksize(void)
{
    struct Process *Process;
    struct CommandLineInterface *CLI;
    ULONG stack;

    Process = (struct Process *) FindTask (0L);
    if ( (CLI = (struct CommandLineInterface *) (Process -> pr_CLI << 2)) )
    {
        stack = CLI -> cli_DefaultStack << 2;
    }
    else
    {
        stack = Process -> pr_StackSize;
    }
    if (stack < MIN_STACKSIZE)
    {
        fprintf (stderr, "*** error: current stack size of %ld bytes is too small for this program, need at least %d bytes.\n", stack, MIN_STACKSIZE);
        exit(EXIT_FAILURE);
    }
}
#endif


int main (int argc, char *argv[])
{

#ifdef __amigaos__
    check_stacksize();
#endif

    printf ("Hello, world!\n");

    return 0;
}

