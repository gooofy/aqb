#include "logger.h"

#include <stdarg.h>
#include <stdio.h>

void runlogger (uint8_t lvl, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    static char buf[1024];
    vsnprintf (buf, 1024, fmt, args);
    va_end(args);
	printf ("%s", buf);
	fflush (stdout);
}

