//#define ENABLE_DPRINTF

#include "Collections.h"

#include <exec/memory.h>

#include <clib/exec_protos.h>
#include <inline/exec.h>

VOID _ArrayListEnumerator_CONSTRUCTOR (ArrayListEnumerator *THIS, ArrayList *list)
{
    THIS->_list           = list;
    THIS->_index          = -1;
    THIS->_currentElement = 0;
}

BOOL _ArrayListEnumerator_MoveNext_ (ArrayListEnumerator *THIS)
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

intptr_t _ArrayListEnumerator_Current_ (ArrayListEnumerator *THIS)
{
    return THIS->_currentElement;
}

VOID _ArrayListEnumerator_Reset (ArrayListEnumerator *THIS)
{
    THIS->_currentElement = NULL;
    THIS->_index = -1;
}

static intptr_t _ArrayListEnumerator_vtable[] = {
    (intptr_t) _CObject_ToString_,
    (intptr_t) _CObject_Equals_,
    (intptr_t) _CObject_GetHashCode_,
    (intptr_t) _ArrayListEnumerator_MoveNext_,
    (intptr_t) _ArrayListEnumerator_Current_,
    (intptr_t) _ArrayListEnumerator_Reset
};

static intptr_t __intf_vtable_ArrayListEnumerator_IEnumerator[] = {
    4,
    (intptr_t) _ArrayListEnumerator_MoveNext_,
    (intptr_t) _ArrayListEnumerator_Current_,
    (intptr_t) _ArrayListEnumerator_Reset
};

void _ArrayListEnumerator___init (ArrayListEnumerator *THIS)
{
    THIS->_vTablePtr = (intptr_t **) &_ArrayListEnumerator_vtable;
    THIS->__intf_vtable_IEnumerator = (intptr_t **) &__intf_vtable_ArrayListEnumerator_IEnumerator;
}

