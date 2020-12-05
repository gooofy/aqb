/*
 * errormsg.c - functions used in all phases of the compiler to give
 *              error messages about the source program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include "util.h"
#include "errormsg.h"
#include "frontend.h"

static bool enable_ansi=FALSE;

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_RESET         "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"

bool EM_anyErrors= FALSE;

bool EM_error(S_pos pos, char *message,...)
{
    va_list ap;

    EM_anyErrors=TRUE;

    if (enable_ansi)
        fprintf(stderr,ANSI_BOLD);
    if (FE_filename) fprintf(stderr,"%s:", FE_filename);
    fprintf(stderr,"%d:%d: ", S_getline(pos), S_getcol(pos));
    if (enable_ansi)
        fprintf(stderr,ANSI_COLOR_RED);
    fprintf(stderr,"error: ");
    if (enable_ansi)
        fprintf(stderr,ANSI_RESET);
    va_start(ap,message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    fprintf(stderr,"\n");

    if (S_getcurlinenum() == S_getline(pos))
    {
        fprintf(stderr, "    %s\n    ", S_getcurline());
        for (int i=1; i<S_getcol(pos); i++)
            fprintf(stderr, " ");
        fprintf(stderr, "^\n");
    }

    return FALSE;
}

string EM_format(S_pos pos, char *message,...)
{
    va_list ap;
    char buf[1024];

    va_start(ap,message);
    vsnprintf(buf, 1024, message, ap);
    va_end(ap);

    if (FE_filename)
        return strprintf("%s:%d:%d: %s", FE_filename, S_getline(pos), S_getcol(pos), buf);
    return strprintf("%d:%d: %s", S_getline(pos), S_getcol(pos), buf);
}

void EM_init(void)
{
    EM_anyErrors = FALSE;
#ifdef __amigaos__
    enable_ansi = FALSE;
#else
    enable_ansi = isatty(fileno(stdout));
#endif
}

