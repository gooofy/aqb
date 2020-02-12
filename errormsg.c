/*
 * errormsg.c - functions used in all phases of the compiler to give
 *              error messages about the source program.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "util.h"
#include "errormsg.h"

extern const char *P_filename;

bool EM_anyErrors= FALSE;

void EM_error(A_pos pos, char *message,...)
{
    va_list ap;
 
    EM_anyErrors=TRUE;
  
    if (P_filename) fprintf(stderr,"%s:", P_filename);
    fprintf(stderr,"%d:%d: ", S_getline(pos), S_getcol(pos));
    va_start(ap,message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    fprintf(stderr,"\n");
}

bool EM_err(char *message,...)
{
    va_list ap;
    A_pos pos = S_getpos();
 
    EM_anyErrors=TRUE;
  
    if (P_filename) fprintf(stderr,"%s:", P_filename);
    fprintf(stderr,"%d:%d: ", S_getline(pos), S_getcol(pos));
    va_start(ap,message);
    vfprintf(stderr, message, ap);
    va_end(ap);
    fprintf(stderr,"\n");

    return FALSE;
}

void EM_init(void)
{
    EM_anyErrors = FALSE; 
}

