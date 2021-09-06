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

#include <inline/exec.h>

#include "tgc.h"

extern struct ExecBase      *SysBase;

static tgc_t gc;

typedef struct
{
    uint16_t stuffing;
    char *myptr;
} myheap_t;

static myheap_t myheap;

//static void f2(void)
//{
//    char *message, *message2;
//
//    printf ("\n------------> f2(): &message=0x%08lx, &message2=0x%08lx\n", (ULONG) &message, (ULONG) &message2);
//}

static void example_function(void)
{
    WORD hubba;
    char *message, *message2;

    printf ("\n------------> example_function(): &message=0x%08lx, &message2=0x%08lx, &hubba=0x%08lx\n",
            (ULONG) &message, (ULONG) &message2, (ULONG) &hubba);
    //f2();
    message = tgc_alloc(&gc, 64, 0);
    strcpy (message, "No More Memory Leaks!");

    printf ("\n------------> example_function(): first tgc_run()\n");
    tgc_run(&gc);
    //myheap.myptr = message;

    printf ("\n------------> example_function(): second alloc\n");
    message2 = tgc_alloc(&gc, 64, 0);
    strcpy (message2, "No More Memory Leaks!");
    tgc_run(&gc);

    printf ("\n------------> example_function(): third alloc\n");
    message = tgc_alloc(&gc, 64, 0);
    strcpy (message, "No More Memory Leaks!");
    tgc_run(&gc);
}

int main (void)
{
	tgc_start(&gc);
    tgc_add_root (&gc, &myheap, sizeof (myheap_t));

	example_function();

    printf ("\n------------> main            (): tgc_stop()\n");
	tgc_stop(&gc);

	return 0;
}

