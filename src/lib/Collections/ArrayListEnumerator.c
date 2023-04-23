//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

VOID _CArrayListEnumerator_CONSTRUCTOR (CArrayListEnumerator *THIS, CArrayList *list)
{
    THIS->_list           = list;
    THIS->_index          = -1;
    THIS->_currentElement = 0;
}

BOOL _CArrayListEnumerator_MoveNext_ (CArrayListEnumerator *THIS)
{
    if (THIS->_index < THIS->_list->_size - 1)
    {
        intptr_t *p = &THIS->_list->_items[++THIS->_index];
        THIS->_currentElement = *p;
        return TRUE;
    }

    THIS->_currentElement = NULL;
    THIS->_index = THIS->_list->_size;
    return FALSE;
}

intptr_t _CArrayListEnumerator_Current_ (CArrayListEnumerator *THIS)
{
    return THIS->_currentElement;
}

VOID _CArrayListEnumerator_Reset (CArrayListEnumerator *THIS)
{
    THIS->_currentElement = NULL;
    THIS->_index = -1;
}

static intptr_t _CArrayListEnumerator_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _CArrayListEnumerator_MoveNext_,
    (intptr_t) _CArrayListEnumerator_Current_,
    (intptr_t) _CArrayListEnumerator_Reset
};

static intptr_t __intf_vtable_CArrayListEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _CArrayListEnumerator_MoveNext_,
    (intptr_t) _CArrayListEnumerator_Current_,
    (intptr_t) _CArrayListEnumerator_Reset
};

void _CArrayListEnumerator___init (CArrayListEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_CArrayListEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_CArrayListEnumerator_IEnumerator;
}

