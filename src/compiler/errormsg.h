#ifndef ERROR_MSG_H
#define ERROR_MSG_H

#include "util.h"
#include "scanner.h"

extern bool      EM_anyErrors;
extern char      EM_firstError[];
extern uint16_t  EM_firstErrorLine;
extern uint16_t  EM_firstErrorCol;

void   EM_init(void);

bool   EM_error(S_pos pos, string, ...); // always returns false

string EM_format(S_pos pos, string, ...);

#endif
