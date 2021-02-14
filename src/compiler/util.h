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
 * endianness
 */

#if defined (__GLIBC__)
#include <endian.h>
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
    #define AQB_LITTLE_ENDIAN
#elif (__BYTE_ORDER == __BIG_ENDIAN)
    #define AQB_BIG_ENDIAN
#elif (__BYTE_ORDER == __PDP_ENDIAN)
    #define AQB_BIG_ENDIAN
#else
    #error "Unable to detect endianness for your target."
#endif
#elif defined(_BIG_ENDIAN)
    #define AQB_BIG_ENDIAN
#elif defined(_LITTLE_ENDIAN)
    #define AQB_LITTLE_ENDIAN
#elif defined(__sparc) || defined(__sparc__) \
   || defined(_POWER) || defined(__powerpc__) \
   || defined(__ppc__) || defined(__hpux) \
   || defined(_MIPSEB) || defined(_POWER) \
   || defined(__s390__)
    #define AQB_BIG_ENDIAN
#elif defined(__i386__) || defined(__alpha__) \
   || defined(__ia64) || defined(__ia64__) \
   || defined(_M_IX86) || defined(_M_IA64) \
   || defined(_M_ALPHA) || defined(__amd64) \
   || defined(__amd64__) || defined(_M_AMD64) \
   || defined(__x86_64) || defined(__x86_64__) \
   || defined(_M_X64)
    #define AQB_LITTLE_ENDIAN
#else
    #error "Unable to detect endianness for your target."
#endif



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
