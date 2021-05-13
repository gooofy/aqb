#ifndef HAVE_COMPILER_H
#define HAVE_COMPILER_H

#include "util.h"

int CO_compile(string sourcefn, string module_name, string symfn, string binfn, string asm_gas_fn, string asm_asmpro_fn);

void CO_exit(int return_code);

#endif
