#ifndef HAVE_PARSER_H
#define HAVE_PARSER_H

#include <stdio.h>
#include "util.h"
#include "absyn.h"

extern const char *P_filename;

bool P_sourceProgram(FILE *inf, const char *filename, A_sourceProgram *sourceProgram);

#endif

