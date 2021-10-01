#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "hashmap.h"

struct S_symbol_
{
    string   name;
    bool     case_sensitive;
};

static S_symbol mksymbol(string name, bool case_sensitive)
{
    S_symbol s=U_poolAlloc (UP_symbol, sizeof(*s));

    s->name           = String(UP_symbol, name);
    s->case_sensitive = case_sensitive;

    return s;
}

static map_t hashtable = NULL;

void SYM_init(void)
{
    hashtable = hashmap_new(UP_symbol);
}

S_symbol S_Symbol(string name, bool case_sensitive)
{
    S_symbol sym;
    int res = hashmap_get(hashtable, name, (any_t *) &sym, case_sensitive);
    if (res != MAP_OK)
    {
        sym = mksymbol(name, case_sensitive);
        hashmap_put(hashtable, name, sym, case_sensitive);
    }
    else
    {
        assert (sym->case_sensitive == case_sensitive);
    }
    return sym;
}

string S_name(S_symbol sym)
{
    return sym->name;
}

/**********************************
 * scopes
 **********************************/

typedef struct S_scopeList_     *S_scopeList;
typedef struct S_scopeListNode_ *S_scopeListNode;

struct S_scope_
{
    TAB_table   tab;
};


S_scope S_beginScope(void)
{
    S_scope s = U_poolAlloc (UP_types, sizeof(*s));

    s->tab    = TAB_empty(UP_types);

    return s;
}

void S_endScope(S_scope s)
{
    // FIXME TAB_free(s->tab);
    // FIXME free(s);
}

void S_enter(S_scope scope, S_symbol sym, void *value)
{
    TAB_enter(scope->tab, sym, value);
}

void *S_look(S_scope s, S_symbol sym)
{
    return TAB_look(s->tab, sym);
}


TAB_iter S_Iter(S_scope scope)
{
    return TAB_Iter(scope->tab);
}

S_symlist S_Symlist(S_symbol sym, S_symlist next)
{
    S_symlist s=U_poolAlloc (UP_frontend, sizeof(*s));

    s->sym  = sym;
    s->next = next;

    return s;
}

