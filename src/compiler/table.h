#ifndef TABLE_H
#define TABLE_H

#include "util.h"

/*
 * table.h - generic hash table
 *
 */

typedef struct TAB_table_ *TAB_table;
typedef struct TAB_iter_  *TAB_iter;

/* Make a new table mapping "keys" to "values". */
TAB_table TAB_empty(U_poolId pid);

/* Enter the mapping "key"->"value" into table "t",
 *    shadowing but not destroying any previous binding for "key". */
void TAB_enter(TAB_table t, void *key, void *value);

/* Look up the most recent binding for "key" in table "t" */
void *TAB_look(TAB_table t, void *key);

/*
 * table iterators
 */

// create a new table iterator
TAB_iter TAB_Iter(TAB_table table);

// get next table element from iterator - returns FALSE when exhausted, TRUE otherwise
bool TAB_next(TAB_iter iter, void **key, void **value);

#endif
