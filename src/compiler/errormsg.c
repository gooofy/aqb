/*
 * errormsg.c - functions used in all phases of the compiler to give
 *              error messages about the source program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>

#include "util.h"
#include "errormsg.h"
#include "parser.h"
#include "logger.h"

static bool enable_ansi=false;

#ifdef __amigaos__

#define ANSI_COLOR_ERROR   "\x9b" "33m"
#define ANSI_RESET         "\x9b" "0m"
#define ANSI_BOLD          "\x9b" "1m"

#else

#define ANSI_COLOR_ERROR   "\x1b[31m"
#define ANSI_RESET         "\x1b[0m"
#define ANSI_BOLD          "\x1b[1m"

#endif

#define MAX_ERROR_LEN 1024

bool     EM_anyErrors = false;
char     EM_firstError[MAX_ERROR_LEN];
uint16_t EM_firstErrorLine;
uint16_t EM_firstErrorCol;

bool EM_error(S_pos pos, char *message,...)
{
    va_list ap;
    static char buf[MAX_ERROR_LEN];

    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_BOLD);
    if (PA_filename) LOG_printf(LOG_ERROR, "%s:", PA_filename);
    LOG_printf(LOG_ERROR, "%d:%d: ", pos.line, pos.col);
    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_COLOR_ERROR);
    LOG_printf(LOG_ERROR, "error: ");
    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_RESET);
    va_start(ap, message);
    vsnprintf (buf, MAX_ERROR_LEN, message, ap);
    va_end(ap);
    LOG_printf(LOG_ERROR, "%s\n", buf);

    LOG_printf(LOG_ERROR,  "    %s\n    ", S_getSourceLine(pos.line));
    for (int i=1; i<pos.col; i++)
        LOG_printf(LOG_ERROR,  " ");
    LOG_printf(LOG_ERROR,  "^\n");

    if (!EM_anyErrors)
    {
        EM_firstErrorLine = pos.line;
        EM_firstErrorCol  = pos.col;
        strncpy (EM_firstError, buf, MAX_ERROR_LEN);
    }

    EM_anyErrors=true;

    return false;
}

string EM_format(S_pos pos, char *message,...)
{
    va_list ap;
    char buf[MAX_ERROR_LEN];

    va_start(ap,message);
    vsnprintf(buf, MAX_ERROR_LEN, message, ap);
    va_end(ap);

    if (PA_filename)
        return strprintf(UP_frontend, "%s:%d:%d: %s", PA_filename, pos.line, pos.col, buf);
    return strprintf(UP_frontend, "%d:%d: %s", pos.line, pos.col, buf);
}

void EM_init(void)
{
    EM_anyErrors = false;
#ifdef __amigaos__
    enable_ansi = true;
#else
    enable_ansi = isatty(fileno(stdout));
#endif
}

