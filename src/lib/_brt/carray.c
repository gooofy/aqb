//#define ENABLE_DPRINTF
#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

VOID _CArray_CONSTRUCTOR (CArray *THIS, LONG elementSize)
{
    THIS->_data        = NULL;
    THIS->_numDims     = 0;
    THIS->_elementSize = elementSize;
    THIS->_dataSize    = 0;
    THIS->_bounds      = NULL;
}

VOID _CArray_REDIM (CArray *THIS, UWORD numDims, BOOL preserve, ...)
{
    va_list valist;

    THIS->_numDims     = numDims;

    THIS->_bounds = ALLOCATE_ (sizeof (CArrayBounds) * numDims, 0);
    if (!THIS->_bounds)
        ERROR (ERR_OUT_OF_MEMORY);

    va_start (valist, preserve);
    ULONG dataSize = THIS->_elementSize;
    for (UWORD iDim=0; iDim<numDims; iDim++)
    {
        ULONG start = va_arg(valist, ULONG);
        ULONG end   = va_arg(valist, ULONG);
        dataSize *= end - start + 1;
        //_debug_puts ("_dyna_redim: dim: start="); _debug_puts2(start); _debug_puts(", end="); _debug_puts2(end); _debug_putnl();
        THIS->_bounds[iDim].lbound      = start;
        THIS->_bounds[iDim].ubound      = end;
        THIS->_bounds[iDim].numElements = end-start+1;
    }
    va_end(valist);

    APTR oData      = THIS->_data;
    ULONG oDataSize = THIS->_dataSize;

    THIS->_data     = ALLOCATE_ (dataSize, 0);
    if (!THIS->_data)
        ERROR (ERR_OUT_OF_MEMORY);
    THIS->_dataSize = dataSize;

    if (oData)
    {
        if (preserve)
        {
            ULONG toCopy = dataSize < oDataSize ? dataSize : oDataSize;
            CopyMem (oData, THIS->_data, toCopy);
            //_debug_puts ("_dyna_redim: preserve, toCopy="); _debug_puts2(toCopy); _debug_putnl();
        }
        DEALLOCATE (oData);
    }
    else
    {
        _CArray_RemoveAll (THIS);
    }
}

intptr_t *_CArray_IDXPTR_ (CArray *THIS, UWORD dimCnt, ...)
{
    if (!THIS->_data)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    if (dimCnt != THIS->_numDims)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    va_list valist;
    va_start (valist, dimCnt);
    ULONG offset = 0;
    ULONG es     = THIS->_elementSize;
    for (WORD iDim=THIS->_numDims-1; iDim>=0; iDim--)
    {
        ULONG lbound = THIS->_bounds[iDim].lbound;
        ULONG ubound = THIS->_bounds[iDim].ubound;
        ULONG n      = THIS->_bounds[iDim].numElements;

        //_debug_puts ("_dyna_idx: dim: iDim="); _debug_puts2(iDim); _debug_puts(", lbound="); _debug_puts2(lbound); _debug_putnl();

        ULONG idx = va_arg(valist, ULONG);

        if ((idx<lbound) || (idx>ubound))
            ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

        offset += es * (idx - lbound);
        es *= n;
        //_debug_puts ("_dyna_idx: dim: idx="); _debug_puts2(idx); _debug_puts(", offset="); _debug_puts2(offset); _debug_putnl();
    }
    va_end(valist);

    BYTE *ptr = THIS->_data + offset;
    DPRINTF ("IDXPTR: THIS=0x%08lx, THIS->_data=0x%08lx, offset=%ld, ptr=0x%08lx\n", THIS, THIS->_data, offset, ptr);

    return (void*) ptr;
}

LONG     _CArray_LBOUND_ (CArray *THIS, WORD     d)
{
    if (d<=0)
        return 1;

    if (!THIS->_data)
        return 0;

    if (d>THIS->_numDims)
        return 0;

    return THIS->_bounds[d-1].lbound;
}

LONG     _CArray_UBOUND_ (CArray *THIS, WORD     d)
{
    if (d<=0)
        return THIS->_numDims;

    if (!THIS->_data)
        return -1;

    if (d>THIS->_numDims)
        return 0;

    return THIS->_bounds[d-1].ubound;
}

VOID _CArray_COPY (CArray *THIS, CArray *a)
{
    if (a->_numDims != THIS->_numDims)
        ERROR (ERR_INCOMPATIBLE_ARRAY);

    LONG toCopy = a->_dataSize < THIS->_dataSize ? a->_dataSize : THIS->_dataSize;

    //DPRINTF ("COPY: toCopy=%ld, a->_dataSize=%ld, THIS->_dataSize=%ld, THIS->_elementSize=%ld\n",
    //         toCopy, a->_dataSize, THIS->_dataSize, THIS->_elementSize);
    //DPRINTF ("COPY: copying %d bytes from 0x%08lx to 0x%08lx\n",
    //         toCopy, a->_data, THIS->_data);

    //BYTE *src = (BYTE*)a->_data;
    //BYTE *dst = (BYTE*)THIS->_data;
    //for (LONG i=0; i<toCopy; i++)
    //{
    //    DPRINTF ("COPY %ld: 0x%08lx = 0x%02x -> 0x%08lx\n", i, src, (int) *src, dst);
    //    *dst++=*src++;
    //}

    CopyMem(a->_data, THIS->_data, toCopy);
}

VOID _CArray_ERASE (CArray *THIS)
{
    if (THIS->_data)
    {
        DEALLOCATE (THIS->_data);
    }
    // FIXME: free bounds!

    THIS->_data        = NULL;
    THIS->_numDims     = 0;
    THIS->_dataSize    = 0;
    THIS->_bounds      = NULL;
}

LONG     _CArray_Count_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Count");
    return 0;
}

VOID _CArray_Capacity (CArray *THIS, LONG     c)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Capacity");
}

LONG     _CArray_Capacity_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Capacity");
    return 0;
}

intptr_t _CArray_GetAt_ (CArray *THIS, LONG     index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.GetAt");
    return 0;
}

VOID _CArray_SetAt (CArray *THIS, LONG     index, intptr_t obj)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.SetAt");
}

intptr_t ** *_CArray_GetEnumerator_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.GetEnumerator");
    return NULL;
}

LONG     _CArray_Add_ (CArray *THIS, intptr_t obj)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Add");
    return 0;
}

BOOL     _CArray_Contains_ (CArray *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Contains");
    return FALSE;
}

VOID _CArray_CLEAR (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.CLEAR");
}

BOOL     _CArray_IsReadOnly_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.IsReadOnly");
    return FALSE;
}

BOOL     _CArray_IsFixedSize_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.IsFixedSize");
    return FALSE;
}

LONG     _CArray_IndexOf_ (CArray *THIS, intptr_t value, LONG     startIndex, LONG     Count)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.IndexOf");
    return 0;
}

VOID _CArray_Insert (CArray *THIS, LONG     index, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Insert");
}

VOID _CArray_Remove (CArray *THIS, intptr_t value)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Remove");
}

VOID _CArray_RemoveAt (CArray *THIS, LONG     index)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.RemoveAt");
}

VOID _CArray_RemoveAll (CArray *THIS)
{
    if (!THIS->_data || !THIS->_dataSize)
        return;

    _MEMSET ((BYTE *)THIS->_data, 0, THIS->_dataSize);
}

CObject *_CArray_Clone_ (CArray *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArray.Clone");
    return NULL;
}

VOID _CArrayEnumerator_CONSTRUCTOR (CArrayEnumerator *THIS, CArray *list)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArrayEnumerator.CONSTRUCTOR");
}

BOOL     _CArrayEnumerator_MoveNext_ (CArrayEnumerator *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArrayEnumerator.MoveNext");
    return FALSE;
}

intptr_t _CArrayEnumerator_Current_ (CArrayEnumerator *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArrayEnumerator.Current");
    return 0;
}

VOID _CArrayEnumerator_Reset (CArrayEnumerator *THIS)
{
    _aqb_assert (FALSE, (STRPTR) "FIXME: implement: CArrayEnumerator.Reset");
}

static intptr_t _CArrayEnumerator_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CArrayEnumerator_MoveNext_,
    (intptr_t) _CArrayEnumerator_Current_,
    (intptr_t) _CArrayEnumerator_Reset
};

static intptr_t __intf_vtable_CArrayEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _CArrayEnumerator_MoveNext_,
    (intptr_t) _CArrayEnumerator_Current_,
    (intptr_t) _CArrayEnumerator_Reset
};

static intptr_t _CArray_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CArray_Count_,
    (intptr_t) _CArray_Capacity_,
    (intptr_t) _CArray_Capacity,
    (intptr_t) _CArray_GetAt_,
    (intptr_t) _CArray_SetAt,
    (intptr_t) _CArray_GetEnumerator_,
    (intptr_t) _CArray_Add_,
    (intptr_t) _CArray_Contains_,
    (intptr_t) _CArray_IsReadOnly_,
    (intptr_t) _CArray_IsFixedSize_,
    (intptr_t) _CArray_IndexOf_,
    (intptr_t) _CArray_Insert,
    (intptr_t) _CArray_Remove,
    (intptr_t) _CArray_RemoveAt,
    (intptr_t) _CArray_RemoveAll,
    (intptr_t) _CArray_Clone_
};

static intptr_t __intf_vtable_CArray_ICloneable[] = {
    16,
    (intptr_t) _CArray_Clone_
};

static intptr_t __intf_vtable_CArray_IEnumerable[] = {
    12,
    (intptr_t) _CArray_GetEnumerator_
};

static intptr_t __intf_vtable_CArray_ICollection[] = {
    8,
    (intptr_t) _CArray_GetEnumerator_,
    (intptr_t) _CArray_Count_
};

static intptr_t __intf_vtable_CArray_IList[] = {
    4,
    (intptr_t) _CArray_GetEnumerator_,
    (intptr_t) _CArray_Count_,
    (intptr_t) _CArray_GetAt_,
    (intptr_t) _CArray_SetAt,
    (intptr_t) _CArray_Add_,
    (intptr_t) _CArray_Contains_,
    (intptr_t) _CArray_IsReadOnly_,
    (intptr_t) _CArray_IsFixedSize_,
    (intptr_t) _CArray_IndexOf_,
    (intptr_t) _CArray_Insert,
    (intptr_t) _CArray_Remove,
    (intptr_t) _CArray_RemoveAt,
    (intptr_t) _CArray_RemoveAll
};


void _CArray___init (CArray *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArray_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_CArray_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_CArray_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_CArray_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_CArray_IList;
}

void _CArrayEnumerator___init (CArrayEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArrayEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_CArrayEnumerator_IEnumerator;
}

