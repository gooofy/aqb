#ifndef HAVE_FRONTEND_H
#define HAVE_FRONTEND_H

#include <stdio.h>
#include "util.h"
#include "env.h"
#include "codegen.h"

extern const char *FE_filename;
extern E_module    FE_mod;

CG_fragList     FE_sourceProgram(FILE *inf, const char *filename, bool is_main, string module_name);

bool            FE_writeSymFile(string symfn);

void            FE_init(void);

void            FE_boot(void);
extern S_symbol FE_keywords[];
extern int      FE_num_keywords;
bool            FE_isKeyword (S_symbol sym);

#endif

