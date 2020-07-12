#ifndef HAVE_AMATH_H
#define HAVE_AMATH_H

void _amath_init(void);

FLOAT mod_(FLOAT divident, FLOAT divisor);

SHORT fix_  (FLOAT f);
SHORT int_  (FLOAT f);
SHORT cint_ (FLOAT f);
LONG  clng_ (FLOAT a);

FLOAT __aqb_shl_single(FLOAT a, FLOAT b);
FLOAT __aqb_shr_single(FLOAT a, FLOAT b);

#endif

