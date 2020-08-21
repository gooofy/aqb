#ifndef HAVE_FRONTEND_H
#define HAVE_FRONTEND_H

#include <stdio.h>
#include "util.h"
#include "env.h"

extern const char *FE_filename;

F_fragList FE_sourceProgram(FILE *inf, const char *filename, bool is_main, string module_name);

bool       FE_writeSymFile(string symfn);

#endif

