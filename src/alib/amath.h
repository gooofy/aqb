#ifndef HAVE_AMATH_H
#define HAVE_AMATH_H

void _amath_init(void);

FLOAT __aqb_mod(FLOAT divident, FLOAT divisor);

SHORT __aqb_fix (FLOAT f);
SHORT __aqb_int (FLOAT f);
SHORT __aqb_cint(FLOAT f);
LONG  __aqb_clng(FLOAT a);

FLOAT __aqb_shl_single(FLOAT a, FLOAT b);
FLOAT __aqb_shr_single(FLOAT a, FLOAT b);

#endif

