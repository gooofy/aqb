//#define ENABLE_DPRINTF
#include "_brt.h"
#include <stdarg.h>

#include <exec/memory.h>

#include <clib/exec_protos.h>

#include <inline/exec.h>

void _CARRAY___gc_scan (CObject *THIS, void *gc)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: _CARRAY___gc_scan");
}

VOID _CARRAY_CONSTRUCTOR (CArray *THIS, LONG elementSize)
{
    THIS->_data        = NULL;
    THIS->_numDims     = 0;
    THIS->_elementSize = elementSize;
    THIS->_dataSize    = 0;
    THIS->_bounds      = NULL;
}

VOID _CARRAY_REDIM (CArray *THIS, UWORD numDims, BOOL preserve, ...)
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
        _CARRAY_REMOVEALL (THIS);
    }
}

intptr_t *_CARRAY_IDXPTR_ (CArray *THIS, UWORD dimCnt, ...)
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

LONG     _CARRAY_LBOUND_ (CArray *THIS, WORD     d)
{
    if (d<=0)
        return 1;

    if (!THIS->_data)
        return 0;

    if (d>THIS->_numDims)
        return 0;

    return THIS->_bounds[d-1].lbound;
}

LONG     _CARRAY_UBOUND_ (CArray *THIS, WORD     d)
{
    if (d<=0)
        return THIS->_numDims;

    if (!THIS->_data)
        return -1;

    if (d>THIS->_numDims)
        return 0;

    return THIS->_bounds[d-1].ubound;
}

VOID _CARRAY_COPY (CArray *THIS, CArray *a)
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

VOID _CARRAY_ERASE (CArray *THIS)
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

LONG _CARRAY_COUNT_ (CArray *THIS)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    return _CARRAY_UBOUND_ (THIS, 1) - _CARRAY_LBOUND_ (THIS, 1) + 1;
}

VOID _CARRAY_CAPACITY (CArray *THIS, LONG     c)
{
    ERROR (ERR_ILLEGAL_FUNCTION_CALL);
}

LONG _CARRAY_CAPACITY_ (CArray *THIS)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    return _CARRAY_UBOUND_ (THIS, 1) - _CARRAY_LBOUND_ (THIS, 1) + 1;
}

intptr_t _CARRAY_GETAT_ (CArray *THIS, LONG index)
{
    DPRINTF ("_CArray_GetAt_: index=%ld\n", index);
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    if ( (index<THIS->_bounds[0].lbound) || (index > THIS->_bounds[0].ubound) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    DPRINTF ("_CArray_GetAt_: bounds[0]=%d..%d\n", THIS->_bounds[0].lbound, THIS->_bounds[0].ubound);
    BYTE *praw = THIS->_data + (index-THIS->_bounds[0].lbound)*THIS->_elementSize;
    switch (THIS->_elementSize)
    {
        case 1:
        {
            return *praw;
        }
        case 2:
        {
            WORD *p = (WORD*)praw;
            return *p;
        }
        case 4:
        {
            LONG *p = (LONG*)praw;
            return *p;
        }
        default:
            ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    }
    return 0;
}

VOID _CARRAY_SETAT (CArray *THIS, LONG index, intptr_t obj)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    if ( (index<THIS->_bounds[0].lbound) || (index > THIS->_bounds[0].ubound) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    BYTE *praw = THIS->_data + (index-THIS->_bounds[0].lbound)*THIS->_elementSize;
    switch (THIS->_elementSize)
    {
        case 1:
        {
            *praw=obj & 0xff;
            break;
        }
        case 2:
        {
            WORD *p = (WORD*)praw;
            *p = obj & 0xffff;
            break;
        }
        case 4:
        {
            LONG *p = (LONG*)praw;
            *p = obj;
            break;
        }
        default:
            ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    }
}

intptr_t ** *_CARRAY_GETENUMERATOR_ (CArray *THIS)
{
    CArrayEnumerator *e = (CArrayEnumerator *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CARRAYENUMERATOR___init (e);
    _CARRAYENUMERATOR_CONSTRUCTOR (e, THIS);

    return &e->__intf_vtable_IEnumerator;
}

LONG _CARRAY_ADD_ (CArray *THIS, intptr_t obj)
{
    ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    return -1;
}

BOOL _CARRAY_CONTAINS_ (CArray *THIS, intptr_t value)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    for ( LONG i=THIS->_bounds[0].lbound; i<= THIS->_bounds[0].ubound; i++)
    {
        intptr_t v = _CARRAY_GETAT_ (THIS, i);
        if (v == value)
            return TRUE;
    }
    return FALSE;
}

BOOL _CARRAY_ISREADONLY_ (CArray *THIS)
{
    return FALSE;
}

BOOL _CARRAY_ISFIXEDSIZE_ (CArray *THIS)
{
    return TRUE;
}

LONG _CARRAY_INDEXOF_ (CArray *THIS, intptr_t value, LONG startIndex, LONG count)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);
    if (startIndex < THIS->_bounds[0].lbound)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    LONG upper_i = count >= 0 ? startIndex + count - 1 : THIS->_bounds[0].ubound;
    for ( LONG i=startIndex; i<= upper_i; i++)
    {
        intptr_t v = _CARRAY_GETAT_ (THIS, i);
        if (v == value)
            return i;
    }
    return -1;
}

VOID _CARRAY_INSERT (CArray *THIS, LONG index, intptr_t value)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);

    if (index < THIS->_bounds[0].lbound)
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    // make room for new element

    for (LONG i=THIS->_bounds[0].ubound; i>index; i--)
    {
        intptr_t v = _CARRAY_GETAT_ (THIS, i-1);
        _CARRAY_SETAT (THIS, i, v);
    }
    _CARRAY_SETAT (THIS, index, value);
}

VOID _CARRAY_REMOVE (CArray *THIS, intptr_t value)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);

    for ( LONG i=THIS->_bounds[0].lbound; i<= THIS->_bounds[0].ubound; i++)
    {
        intptr_t v = _CARRAY_GETAT_ (THIS, i);
        if (v == value)
        {
            _CARRAY_REMOVEAT (THIS, i);
            return;
        }
    }
}

VOID _CARRAY_REMOVEAT (CArray *THIS, LONG index)
{
    if (THIS->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);

    LONG lbound = THIS->_bounds[0].lbound;
    LONG ubound = THIS->_bounds[0].ubound;

    if ( (index<lbound) || (index>ubound) )
        ERROR (ERR_SUBSCRIPT_OUT_OF_RANGE);

    for (LONG i=index+1; i<=ubound; i++)
    {
        _CARRAY_SETAT (THIS, i-1, _CARRAY_GETAT_ (THIS, i));
    }
    _CARRAY_SETAT (THIS, ubound, 0);
}

VOID _CARRAY_REMOVEALL (CArray *THIS)
{
    if (!THIS->_data || !THIS->_dataSize)
        return;

    _MEMSET ((BYTE *)THIS->_data, 0, THIS->_dataSize);
}

CObject *_CARRAY_CLONE_ (CArray *THIS)
{
    CArray *e = (CArray *)ALLOCATE_(sizeof (*e), MEMF_ANY);
    if (!e)
        ERROR (ERR_OUT_OF_MEMORY);

    _CARRAY___init (e);
    _CARRAY_CONSTRUCTOR (e, THIS->_elementSize);

    e->_data = ALLOCATE_ (THIS->_dataSize, MEMF_ANY);
    if (!e->_data)
        ERROR (ERR_OUT_OF_MEMORY);
    e->_dataSize = THIS->_dataSize;
    CopyMem (THIS->_data, e->_data, THIS->_dataSize);

    e->_numDims = THIS->_numDims;
    e->_bounds = ALLOCATE_ (sizeof (CArrayBounds) * e->_numDims, 0);
    if (!e->_bounds)
        ERROR (ERR_OUT_OF_MEMORY);

    for (UWORD iDim=0; iDim<THIS->_numDims; iDim++)
    {
        e->_bounds[iDim].lbound      = THIS->_bounds[iDim].lbound;
        e->_bounds[iDim].ubound      = THIS->_bounds[iDim].ubound;
        e->_bounds[iDim].numElements = THIS->_bounds[iDim].numElements;
    }

    return (CObject*) e;
}

void _CARRAYENUMERATOR___gc_scan (CObject *THIS, void *gc)
{
    _AQB_ASSERT (FALSE, (STRPTR) "FIXME: implement: _CARRAYENUMERATOR___gc_scan");
}

VOID _CARRAYENUMERATOR_CONSTRUCTOR (CArrayEnumerator *THIS, CArray *array)
{
    THIS->_array          = array;
    LONG lbound = THIS->_array->_bounds[0].lbound;
    THIS->_index          = lbound-1;
    THIS->_currentElement = 0;
}

BOOL _CARRAYENUMERATOR_MOVENEXT_ (CArrayEnumerator *THIS)
{
    if (THIS->_array->_numDims != 1)
        ERROR (ERR_ILLEGAL_FUNCTION_CALL);

    LONG ubound = THIS->_array->_bounds[0].ubound;

    if (THIS->_index < ubound)
    {
        THIS->_currentElement = _CARRAY_GETAT_ (THIS->_array, ++THIS->_index);
        return TRUE;
    }

    THIS->_currentElement = NULL;
    THIS->_index = ubound;
    return FALSE;
}

intptr_t _CARRAYENUMERATOR_CURRENT_ (CArrayEnumerator *THIS)
{
    return THIS->_currentElement;
}

VOID _CARRAYENUMERATOR_RESET (CArrayEnumerator *THIS)
{
    THIS->_currentElement = NULL;
    THIS->_index = -1;
}

#if 0
static intptr_t _CArrayEnumerator_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CARRAYENUMERATOR_MOVENEXT_,
    (intptr_t) _CARRAYENUMERATOR_CURRENT_,
    (intptr_t) _CARRAYENUMERATOR_RESET
};

static intptr_t __intf_vtable_CArrayEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _CARRAYENUMERATOR_MOVENEXT_,
    (intptr_t) _CARRAYENUMERATOR_CURRENT_,
    (intptr_t) _CARRAYENUMERATOR_RESET
};

static intptr_t _CArray_vtable[] = {
    (intptr_t) _COBJECT_TOSTRING_,
    (intptr_t) _COBJECT_EQUALS_,
    (intptr_t) _COBJECT_GETHASHCODE_,
    (intptr_t) _CARRAY_COUNT_,
    (intptr_t) _CARRAY_CAPACITY_,
    (intptr_t) _CARRAY_CAPACITY,
    (intptr_t) _CARRAY_GETAT_,
    (intptr_t) _CARRAY_SETAT,
    (intptr_t) _CARRAY_GETENUMERATOR_,
    (intptr_t) _CARRAY_ADD_,
    (intptr_t) _CARRAY_CONTAINS_,
    (intptr_t) _CARRAY_ISREADONLY_,
    (intptr_t) _CARRAY_ISFIXEDSIZE_,
    (intptr_t) _CARRAY_INDEXOF_,
    (intptr_t) _CARRAY_INSERT,
    (intptr_t) _CARRAY_REMOVE,
    (intptr_t) _CARRAY_REMOVEAT,
    (intptr_t) _CARRAY_REMOVEALL,
    (intptr_t) _CARRAY_CLONE_
};

static intptr_t __intf_vtable_CArray_ICloneable[] = {
    16,
    (intptr_t) _CARRAY_CLONE_
};

static intptr_t __intf_vtable_CArray_IEnumerable[] = {
    12,
    (intptr_t) _CARRAY_GETENUMERATOR_
};

static intptr_t __intf_vtable_CArray_ICollection[] = {
    8,
    (intptr_t) _CARRAY_GETENUMERATOR_,
    (intptr_t) _CARRAY_COUNT_
};

static intptr_t __intf_vtable_CArray_IList[] = {
    4,
    (intptr_t) _CARRAY_GETENUMERATOR_,
    (intptr_t) _CARRAY_COUNT_,
    (intptr_t) _CARRAY_GETAT_,
    (intptr_t) _CARRAY_SETAT,
    (intptr_t) _CARRAY_ADD_,
    (intptr_t) _CARRAY_CONTAINS_,
    (intptr_t) _CARRAY_ISREADONLY_,
    (intptr_t) _CARRAY_ISFIXEDSIZE_,
    (intptr_t) _CARRAY_INDEXOF_,
    (intptr_t) _CARRAY_INSERT,
    (intptr_t) _CARRAY_REMOVE,
    (intptr_t) _CARRAY_REMOVEAT,
    (intptr_t) _CARRAY_REMOVEALL
};


void _CARRAY___init (CArray *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArray_vtable;
    THIS->__intf_vtable_ICloneable = (intptr_t **) &__intf_vtable_CArray_ICloneable;
    THIS->__intf_vtable_IEnumerable = (intptr_t **) &__intf_vtable_CArray_IEnumerable;
    THIS->__intf_vtable_ICollection = (intptr_t **) &__intf_vtable_CArray_ICollection;
    THIS->__intf_vtable_IList = (intptr_t **) &__intf_vtable_CArray_IList;
}

void _CARRAYENUMERATOR___init (CArrayEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArrayEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_CArrayEnumerator_IEnumerator;
}
#endif
