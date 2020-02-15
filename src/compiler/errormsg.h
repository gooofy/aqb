#ifndef ERROR_MSG_H
#define ERROR_MSG_H

#include "util.h"
#include "scanner.h"

extern bool EM_anyErrors;

void EM_init(void);

bool EM_err(string,...); // always returns FALSE
void EM_error(A_pos pos, string,...); 

#endif
