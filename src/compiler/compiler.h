#ifndef HAVE_COMPILER_H
#define HAVE_COMPILER_H

#include "util.h"
#include "ir.h"

IR_assembly CO_AssemblyInit  (S_symbol name);

void        CO_AssemblyParse (IR_assembly assembly, string sourcefn);

//int CO_compile(string sourcefn, string module_name, string symfn, string cstubfn, string objfn, string binfn,
//               string asm_gas_fn, string asm_asmpro_fn, string asm_vasm_fn, bool hasCode, bool noInitFn,
//               bool gcScanExtern);

void CO_exit(int return_code);

#endif
