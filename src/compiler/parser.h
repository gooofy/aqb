#ifndef HAVE_PARSER_H
#define HAVE_PARSER_H

#include <stdio.h>
#include "util.h"
#include "absyn.h"
#include "env.h"

extern const char *P_filename;

bool P_sourceProgram(FILE *inf, const char *filename, A_sourceProgram *sourceProgram);

bool P_functionCall(S_tkn *tkn, P_declProc dec, A_exp *exp);

bool P_subCall(S_tkn *tkn, P_declProc dec);

#endif

