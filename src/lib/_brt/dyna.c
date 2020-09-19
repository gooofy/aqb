#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

void _dyna_init (_dyna *a, UWORD numDims, ULONG elementSize, ...)
{
    va_list valist;

    a->data        = NULL;
    a->numDims     = numDims;
    a->elementSize = elementSize;
    a->bounds      = NULL;

    if (numDims>0)
    {
        a->bounds = ALLOCATE_ (sizeof (_dyna_bounds) * numDims, 0);

        va_start (valist, elementSize);
        ULONG dataSize = elementSize;
        for (UWORD iDim=0; iDim<numDims; iDim++)
        {
            ULONG start = va_arg(valist, ULONG);
            ULONG end   = va_arg(valist, ULONG);
            dataSize *= end - start + 1;
            //_debug_puts ("_dyna_create: dim: start="); _debug_puts2(start); _debug_puts(", end="); _debug_puts2(end); _debug_putnl();
            a->bounds[iDim].lbound      = start;
            a->bounds[iDim].ubound      = end;
            a->bounds[iDim].numElements = end-start+1;
        }
        va_end(valist);
        //_debug_puts ("_dyna_create: dataSize="); _debug_puts2(dataSize); _debug_putnl();
        a->data = ALLOCATE_ (dataSize, 0);
    }
}

//int foobar(int a)
//{
//    return a*a;
//}

/*
 * elements are in row-major order
 * array dimensions: a(N0, N1, N2, ...), lement size E
 * e.g. DIM a(4, 3), E=4          / e_0_0[ 0]  e_0_1[ 4]  e_0_2[ 8] \
 *                               /  e_1_0[12]  e_1_1[16]  e_1_2[20]  \
 *                               \  e_2_0[24]  e_2_1[28]  e_2_2[32]  /
 *                                \ e_3_0[36]  e_3_1[40]  e_3_2[44] /
 *
 * offset calc for a(i0, i1)        , element size E : o = E*i1 + E*N1*i0
 *                 a(i0, i1, i2, i3)                   o = E*i3 + E*N3*i2 + E*N2*N3*i1 + E*N1*N2*N3*i0
 *
 *
 *
 */


void *_dyna_idx_(_dyna *dyna, UWORD dimCnt, ...)
{
    //int i = foobar(dimCnt);

    //_dyna_create_ (dimCnt, i, 1, 2);

    //_debug_puts ("_dyna_idx: dimCnt="); _debug_puts2(dimCnt); _debug_putnl();

    if (!dyna->data)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    if (dimCnt != dyna->numDims)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    va_list valist;
    va_start (valist, dimCnt);
    ULONG offset = 0;
    ULONG es     = dyna->elementSize;
    for (WORD iDim=dyna->numDims-1; iDim>=0; iDim--)
    {
        ULONG lbound = dyna->bounds[iDim].lbound;
        ULONG ubound = dyna->bounds[iDim].ubound;
        ULONG n     = dyna->bounds[iDim].numElements;

        // _debug_puts ("_dyna_idx: dim: iDim="); _debug_puts2(iDim); _debug_puts(", lbound="); _debug_puts2(lbound); _debug_putnl();

        ULONG idx = va_arg(valist, ULONG);

        if ((idx<lbound) || (idx>ubound))
            ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

        offset += es * (idx - lbound);
        es *= n;
        //_debug_puts ("_dyna_idx: dim: idx="); _debug_puts2(idx); _debug_puts(", offset="); _debug_puts2(offset); _debug_putnl();
    }
    va_end(valist);

    return dyna->data + offset;
}

WORD _dyna_lbound_ (_dyna *dyna, WORD d)
{
    if (d<=0)
        return 1;

    if (!dyna->data)
        return 0;

    if (d>dyna->numDims)
        return 0;

    return dyna->bounds[d-1].lbound;
}
WORD _dyna_ubound_ (_dyna *dyna, WORD d)
{
    if (d<=0)
        return dyna->numDims;

    if (!dyna->data)
        return -1;

    if (d>dyna->numDims)
        return 0;

    return dyna->bounds[d-1].ubound;
}
