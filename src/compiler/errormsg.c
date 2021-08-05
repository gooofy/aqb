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
#include "frontend.h"
#include "logger.h"

static bool enable_ansi=FALSE;

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

bool     EM_anyErrors = FALSE;
char     EM_firstError[MAX_ERROR_LEN];
uint16_t EM_firstErrorLine;
uint16_t EM_firstErrorCol;

bool EM_error(S_pos pos, char *message,...)
{
    va_list ap;
    static char buf[MAX_ERROR_LEN];

    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_BOLD);
    if (FE_filename) LOG_printf(LOG_ERROR, "%s:", FE_filename);
    LOG_printf(LOG_ERROR, "%d:%d: ", S_getline(pos), S_getcol(pos));
    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_COLOR_ERROR);
    LOG_printf(LOG_ERROR, "error: ");
    if (enable_ansi)
        LOG_printf(LOG_ERROR, ANSI_RESET);
    va_start(ap, message);
    vsnprintf (buf, MAX_ERROR_LEN, message, ap);
    va_end(ap);
    LOG_printf(LOG_ERROR, "%s\n", buf);

    if (S_getcurlinenum() == S_getline(pos))
    {
        LOG_printf(LOG_ERROR,  "    %s\n    ", S_getcurline());
        for (int i=1; i<S_getcol(pos); i++)
            LOG_printf(LOG_ERROR,  " ");
        LOG_printf(LOG_ERROR,  "^\n");
    }

    if (!EM_anyErrors)
    {
        EM_firstErrorLine = S_getline(pos);
        EM_firstErrorCol  = S_getcol(pos);
        strncpy (EM_firstError, buf, MAX_ERROR_LEN);
    }

    EM_anyErrors=TRUE;

    return FALSE;
}

string EM_format(S_pos pos, char *message,...)
{
    va_list ap;
    char buf[MAX_ERROR_LEN];

    va_start(ap,message);
    vsnprintf(buf, MAX_ERROR_LEN, message, ap);
    va_end(ap);

    if (FE_filename)
        return strprintf(UP_frontend, "%s:%d:%d: %s", FE_filename, S_getline(pos), S_getcol(pos), buf);
    return strprintf(UP_frontend, "%d:%d: %s", S_getline(pos), S_getcol(pos), buf);
}

void EM_init(void)
{
    EM_anyErrors = FALSE;
#ifdef __amigaos__
    enable_ansi = TRUE;
#else
    enable_ansi = isatty(fileno(stdout));
#endif
}

