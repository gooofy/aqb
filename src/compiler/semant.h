#ifndef SEMANT_H
#define SEMANT_H

#include "translate.h"
#include "absyn.h"

F_fragList SEM_transProg(A_sourceProgram sourceProgram, bool is_main, string module_name);

bool       SEM_writeSymFile(string symfn);

#endif
