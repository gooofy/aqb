#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

void __DARRAY_T___init__ (_DARRAY_T *self, ULONG elementSize)
{
    // _debug_puts ("__DARRAY_T___init__: elementSize="); _debug_puts2(elementSize); _debug_putnl();
    self->data        = NULL;
    self->numDims     = 0;
    self->elementSize = elementSize;
    self->dataSize    = 0;
    self->bounds      = NULL;
}

void __DARRAY_T_REDIM (_DARRAY_T *self, BOOL preserve, UWORD numDims, ...)
{
    va_list valist;

    self->numDims     = numDims;

    self->bounds = ALLOCATE_ (sizeof (_DARRAY_BOUNDS_T) * numDims, 0);

    va_start (valist, numDims);
    ULONG dataSize = self->elementSize;
    for (UWORD iDim=0; iDim<numDims; iDim++)
    {
        ULONG start = va_arg(valist, ULONG);
        ULONG end   = va_arg(valist, ULONG);
        dataSize *= end - start + 1;
        //_debug_puts ("_dyna_redim: dim: start="); _debug_puts2(start); _debug_puts(", end="); _debug_puts2(end); _debug_putnl();
        self->bounds[iDim].lbound      = start;
        self->bounds[iDim].ubound      = end;
        self->bounds[iDim].numElements = end-start+1;
    }
    va_end(valist);

    APTR oData      = self->data;
    ULONG oDataSize = self->dataSize;

    self->data     = ALLOCATE_ (dataSize, 0);
    self->dataSize = dataSize;

    if (oData)
    {
        if (preserve)
        {
            ULONG toCopy = dataSize < oDataSize ? dataSize : oDataSize;
            CopyMem (oData, self->data, toCopy);
            //_debug_puts ("_dyna_redim: preserve, toCopy="); _debug_puts2(toCopy); _debug_putnl();
        }
        DEALLOCATE (oData, oDataSize);
    }

    //_debug_puts ("_dyna_redim: self="); _debug_putu4((ULONG) self); _debug_puts(", data="); _debug_putu4((ULONG)self->data); _debug_puts (", dataSize="); _debug_puts2(dataSize); _debug_puts (", numDims="); _debug_puts2(numDims); _debug_putnl();
}

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

void *__DARRAY_T_IDXPTR_ (_DARRAY_T *self, UWORD dimCnt, ...)
{
    //int i = foobar(dimCnt);

    //_dyna_create_ (dimCnt, i, 1, 2);

    // _debug_puts ("_dyna_idx: dimCnt="); _debug_puts2(dimCnt); _debug_putnl();

    if (!self->data)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    if (dimCnt != self->numDims)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    va_list valist;
    va_start (valist, dimCnt);
    ULONG offset = 0;
    ULONG es     = self->elementSize;
    for (WORD iDim=self->numDims-1; iDim>=0; iDim--)
    {
        ULONG lbound = self->bounds[iDim].lbound;
        ULONG ubound = self->bounds[iDim].ubound;
        ULONG n     = self->bounds[iDim].numElements;

        //_debug_puts ("_dyna_idx: dim: iDim="); _debug_puts2(iDim); _debug_puts(", lbound="); _debug_puts2(lbound); _debug_putnl();

        ULONG idx = va_arg(valist, ULONG);

        if ((idx<lbound) || (idx>ubound))
            ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

        offset += es * (idx - lbound);
        es *= n;
        //_debug_puts ("_dyna_idx: dim: idx="); _debug_puts2(idx); _debug_puts(", offset="); _debug_puts2(offset); _debug_putnl();
    }
    va_end(valist);

    void *ptr = self->data+offset;
    //_debug_puts ("_dyna_idx: self="); _debug_putu4((ULONG) self); _debug_puts(", data="); _debug_putu4((ULONG)self->data); _debug_puts (" -> ptr="); _debug_putu4((ULONG)ptr); _debug_putnl();

    return ptr;
}

WORD  __DARRAY_T_LBOUND_  (_DARRAY_T *self, WORD d)
{
    if (d<=0)
        return 1;

    if (!self->data)
        return 0;

    if (d>self->numDims)
        return 0;

    return self->bounds[d-1].lbound;
}

WORD  __DARRAY_T_UBOUND_  (_DARRAY_T *self, WORD d)
{
    if (d<=0)
        return self->numDims;

    if (!self->data)
        return -1;

    if (d>self->numDims)
        return 0;

    return self->bounds[d-1].ubound;
}

void __DARRAY_T_COPY (_DARRAY_T *self, _DARRAY_T *a)
{
    if (a->numDims != self->numDims)
        ERROR (ERR_INCOMPATIBLE_ARRAY);

    ULONG toCopy = a->dataSize < self->dataSize ? a->dataSize : self->dataSize;

    //_debug_puts ("__DARRAY_T_COPY: toCopy="); _debug_puts2(toCopy); _debug_putnl();
    CopyMem(a->data, self->data, toCopy);
}

void __DARRAY_T_ERASE (_DARRAY_T *self)
{
    if (self->data)
    {
        DEALLOCATE (self->data, self->dataSize);
    }
    // FIXME: free bounds!

    self->data        = NULL;
    self->numDims     = 0;
    self->dataSize    = 0;
    self->bounds      = NULL;
}

