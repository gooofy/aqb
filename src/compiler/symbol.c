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
    map_t     map;
};

S_scope S_beginScope(void)
{
    S_scope s = checked_malloc(sizeof(*s));

    s->map        = hashmap_new();

    return s;
}

void S_endScope(S_scope s)
{
    hashmap_free(s->map);
    free(s);
}

void S_enter(S_scope scope, S_symbol sym, void *value)
{
    hashmap_put(scope->map, sym->name, value);
}

void *S_look(S_scope s, S_symbol sym)
{
    any_t res;
    if (hashmap_get(s->map, sym->name, &res)==MAP_OK)
        return res;
    return NULL;
}

#if 0
void S_dump(S_table t, void (*show)(S_symbol sym, void *binding)) {
    TAB_dump(t, (void (*)(void *, void *)) show);
}
#endif

