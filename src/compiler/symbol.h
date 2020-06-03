#ifndef SYMBOL_H
#define SYMBOL_H

#include "hashmap.h"

/*
 * symbol.h - Symbols and symbol-tables
 *
 */

void S_symbol_init(void);

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
typedef struct S_scope_ *S_scope;

/* Start a new scope. Scopes can be nested. */
S_scope S_beginScope(S_scope parent);

S_scope S_parent(S_scope s);

/* free scope s */
void S_endScope(S_scope s);

/* enter a binding "sym->value" into "scope" */
void S_enter(S_scope scope, S_symbol sym, void *value);

/* look up the most recent binding of "sym" in "scope", or return NULL if sym is unbound. */
void *S_look(S_scope scope, S_symbol sym);

#endif
