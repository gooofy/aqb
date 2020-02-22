#ifndef HAVE_ASTRING_H
#define HAVE_ASTRING_H

#include "autil.h"

void _astr_itoa(int num, char *str, int base);

ULONG _astr_len(char *str);

char *_astr_dup(char *str);

#endif

