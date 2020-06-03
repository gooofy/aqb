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
    S_symbol s=checked_malloc(sizeof(*s));

    s->name           = String(name);
    s->case_sensitive = case_sensitive;

    return s;
}

static map_t hashtable = NULL;

void S_symbol_init(void)
{
    if (hashtable)
        hashmap_free(hashtable);
    hashtable = hashmap_new();
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
    return sym;
}

string S_name(S_symbol sym)
{
    return sym->name;
}

/**********************************
 * scopes
 **********************************/

struct S_scope_
{
    S_scope   parent;
    map_t     map;
};

S_scope S_beginScope(S_scope parent)
{
    S_scope s = checked_malloc(sizeof(*s));

    s->map     = hashmap_new();
    s->parent  = parent;

    return s;
}

S_scope S_parent(S_scope s)
{
    return s->parent;
}

void S_endScope(S_scope s)
{
    hashmap_free(s->map);
    free(s);
}

void S_enter(S_scope scope, S_symbol sym, void *value)
{
    hashmap_put(scope->map, sym->name, value, sym->case_sensitive);
}

void *S_look(S_scope s, S_symbol sym)
{
    any_t res;
    if (hashmap_get(s->map, sym->name, &res, sym->case_sensitive)==MAP_OK)
        return res;
    return NULL;
}

