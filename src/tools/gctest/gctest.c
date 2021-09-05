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

static void example_function()
{
    char *message = tgc_alloc(&gc, 64, 0);
    strcpy (message, "No More Memory Leaks!");
    myheap.myptr = message;
    message = tgc_alloc(&gc, 64, 0);
    strcpy (message, "No More Memory Leaks!");
}

int main (void)
{
	int foo = 42;

    printf ("GC Test program\n");

	tgc_start(&gc, &foo);
    tgc_add_root (&gc, &myheap, sizeof (myheap_t));

	example_function();

	tgc_run(&gc);
	tgc_stop(&gc);

	return 0;
}

