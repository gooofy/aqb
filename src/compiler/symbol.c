#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "symbol.h"
#include "hashmap.h"

struct S_symbol_ 
{
    string   name; 
};

static S_symbol mksymbol(string name)
{
    S_symbol s=checked_malloc(sizeof(*s));
    s->name=name; 
    return s;
}

static map_t hashtable = NULL;

void S_symbol_init(void)
{
    if (hashtable)
        hashmap_free(hashtable);
    hashtable = hashmap_new();
}

S_symbol S_Symbol(string name)
{
    S_symbol sym;
    int res = hashmap_get(hashtable, name, (any_t *) &sym);
    if (res != MAP_OK)
    {
        sym = mksymbol(name);
        hashmap_put(hashtable, name, sym);
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
    S_scope parent;
    map_t   map;
};

S_scope S_beginScope(S_scope parent)
{
    S_scope s = checked_malloc(sizeof(*s));

    s->map    = hashmap_new();
    s->parent = parent;

    return s;
}

S_scope S_endScope(S_scope s)
{
    S_scope parent = s->parent;

    hashmap_free(s->map);
    free(s);

    return parent;
}

void S_enter(S_scope scope, S_symbol sym, void *value) 
{
    hashmap_put(scope->map, sym->name, value);
}

void *S_look(S_scope s, S_symbol sym) 
{
    S_scope scope = s;

    while (scope)
    {
        any_t res;
        if (hashmap_get(scope->map, sym->name, &res)==MAP_OK)
            return res;
        scope = scope->parent;
    }
    return NULL;
}

#if 0
void S_dump(S_table t, void (*show)(S_symbol sym, void *binding)) {
    TAB_dump(t, (void (*)(void *, void *)) show);
}
#endif

