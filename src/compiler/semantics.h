#ifndef HAVE_SEMANTICS_H
#define HAVE_SEMANTICS_H

#include "ir.h"

void SEM_elaborate (IR_assembly assembly, IR_namespace names_root);
void SEM_boot      (void);

#endif // HAVE_SEMANTICS_H
