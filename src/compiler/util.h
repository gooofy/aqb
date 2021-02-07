#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef char *string;
typedef char bool;

#define TRUE  1
#define FALSE 0

/*
 * memory management
 */

typedef enum
{
    UP_frontend, UP_types, UP_temp, UP_assem, UP_codegen, UP_env, UP_flowgraph, UP_linscan, UP_symbol, 
    UP_hashmap, UP_regalloc, UP_liveness, UP_table, UP_strings, UP_numPools
} U_poolId;

void *U_poolAlloc  (U_poolId pid, size_t size);
void *U_poolCalloc (U_poolId pid, size_t nmemb, size_t len);
void  U_poolReset  (U_poolId pid);   // frees all memory reserved through this pool, keeps pool itself intact for now allocations

void *U_malloc     (size_t size);
void *U_calloc     (size_t nmemb, size_t len);

void  U_memstat(void);

/*
 * string support
 */

string String(const char *); // allocs mem + copies string

string strconcat(const char *s1, const char*s2); // allocates mem for concatenated string

string strprintf(const char *format, ...); // allocates mem for resulting string
string strlower(const char *s);  // allocates mem for resulting string

int    strcicmp(string a, string b); // string ignore case compare

void   strserialize(FILE *out, string str);
string strdeserialize(FILE *in);

/*
 * FFP - Motorola Fast Floating Point format support
 */

uint32_t encode_ffp(float        f);
float    decode_ffp(uint32_t fl);


void U_init (void);

#endif
