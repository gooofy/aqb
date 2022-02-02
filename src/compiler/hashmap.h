/*
 * Generic hashmap manipulation functions
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "util.h"

#define MAP_MISSING -3  /* No such element */
#define MAP_FULL    -2 	/* Hashmap is full */
#define MAP_OMEM    -1 	/* Out of Memory */
#define MAP_OK       0 	/* OK */

/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void *any_t;

/*
 * map_t is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef any_t map_t;

/* create an empty hashmap */
map_t hashmap_new(U_poolId pid);

/* add an element to the hashmap. Return MAP_OK or MAP_OMEM.  */
int hashmap_put(map_t in, char* key, any_t value, bool copy_key);

/* get an element from the hashmap. Return MAP_OK or MAP_MISSING.  */
int hashmap_get(map_t in, char* key, any_t *value);

/* remove an element from the hashmap. Return MAP_OK or MAP_MISSING.  */
int hashmap_remove(map_t in, char* key);

/* get the current size of a hashmap */
extern int hashmap_length(map_t in);

#endif

