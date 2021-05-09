#ifndef SYMBOL_H
#define SYMBOL_H

#include "table.h"

/*
 * symbol.h - Symbols and symbol-tables
 *
 */

void SYM_init(void);

typedef struct S_symbol_ *S_symbol;

/* make a unique symbol from a given string.
 *  Different calls to S_Symbol("foo") will yield the same S_symbol
 *  value, even if the "foo" strings are at different locations. */
S_symbol S_Symbol(string s, bool case_sensitive);

/* extract the underlying string from a symbol */
string S_name(S_symbol);

/*
 * scopes
 */

/* scopes are nested, map symbol -> anyptr */
typedef struct S_scope_     *S_scope;

/* Start a new scope */
S_scope S_beginScope(void);

/* free scope s */
void S_endScope(S_scope s);

/* enter a binding "sym->value" into "scope" */
void S_enter(S_scope scope, S_symbol sym, void *value);

/* look up the most recent binding of "sym" in "scope", or return NULL if sym is unbound. */
void *S_look(S_scope scope, S_symbol sym);

// iterate through all entries in scope
TAB_iter S_Iter(S_scope scope);

/*
 * list of symbols
 */

typedef struct S_symlist_ *S_symlist;

struct S_symlist_
{
    S_symbol  sym;
    S_symlist next;
};

S_symlist S_Symlist(S_symbol sym, S_symlist next);

#endif
