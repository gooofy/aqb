#ifndef HAVE_COMPILER_H
#define HAVE_COMPILER_H

#include "util.h"

int CO_compile(string sourcefn, string module_name, string symfn, string cstubfn, string objfn, string binfn,
               string asm_gas_fn, string asm_asmpro_fn, string asm_vasm_fn, bool hasCode, bool noInitFn);

void CO_exit(int return_code);

#endif
