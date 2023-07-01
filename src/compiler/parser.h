#ifndef HAVE_PARSER_H
#define HAVE_PARSER_H

#include <stdio.h>
#include "ir.h"

extern const char *PA_filename;

void PA_compilation_unit(IR_assembly assembly, IR_namespace names_root, FILE *sourcef, const char *sourcefn);

void PA_boot(void);
void PA_init(void);


#endif
