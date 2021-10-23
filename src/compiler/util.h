#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __amigaos__
#include <intuition/intuition.h>
#endif

typedef char *string;
typedef char bool;

#define TRUE  1
#define FALSE 0

#define VERSION            "0.7.4"
#define COPYRIGHT          "(C) 2020, 2021 by G. Bartsch"
#define PROGRAM_NAME_SHORT "AQB"
#define PROGRAM_NAME_LONG  "AQB Amiga BASIC"
#define LICENSE            "Licensed under the MIT license."

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

#ifdef AQB_LITTLE_ENDIAN

#define ENDIAN_SWAP_16(data) ( (((data) >> 8) & 0x00FF) | (((data) << 8) & 0xFF00) )
#define ENDIAN_SWAP_32(data) ( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
                               (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) )
#define ENDIAN_SWAP_64(data) ( (((data) & 0x00000000000000ffLL) << 56) | \
                               (((data) & 0x000000000000ff00LL) << 40) | \
                               (((data) & 0x0000000000ff0000LL) << 24) | \
                               (((data) & 0x00000000ff000000LL) << 8)  | \
                               (((data) & 0x000000ff00000000LL) >> 8)  | \
                               (((data) & 0x0000ff0000000000LL) >> 24) | \
                               (((data) & 0x00ff000000000000LL) >> 40) | \
                               (((data) & 0xff00000000000000LL) >> 56))

#else

#define ENDIAN_SWAP_16(data) (data)
#define ENDIAN_SWAP_32(data) (data)
#define ENDIAN_SWAP_64(data) (data)

#endif

static inline int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

/*
 * memory management
 */

typedef enum
{
    UP_frontend, UP_types, UP_temp, UP_assem, UP_codegen, UP_env, UP_flowgraph, UP_linscan, UP_symbol,
    UP_regalloc, UP_liveness, UP_link, UP_ide, UP_options, UP_numPools
} U_poolId;

void *U_poolAlloc          (U_poolId pid, size_t size);
void *U_poolCalloc         (U_poolId pid, size_t nmemb, size_t len);
void *U_poolNonChunkAlloc  (U_poolId pid, size_t size);
void *U_poolNonChunkCAlloc (U_poolId pid, size_t size);
void  U_poolReset          (U_poolId pid);   // frees all memory reserved through this pool, keeps pool itself intact for now allocations

void  U_memstat    (void);

/*
 * string support
 */

string String         (U_poolId pid, const char *); // allocs mem + copies string

string strconcat      (U_poolId pid, const char *s1, const char*s2); // allocates mem for concatenated string

string strprintf      (U_poolId pid, const char *format, ...); // allocates mem for resulting string
string strlower       (U_poolId pid, const char *s);  // allocates mem for resulting string

int    strcicmp       (string a, string b); // string ignore case compare

void   strserialize   (FILE *out, string str);
string strdeserialize (U_poolId pid, FILE *in);

bool   str2int        (string str, int *i);

/*
 * FFP - Motorola Fast Floating Point format support
 */

uint32_t encode_ffp (float        f);
float    decode_ffp (uint32_t fl);

void     U_float2str(double v, char *buffer, int buf_len);

/*
 * misc
 */

float    U_getTime  (void);
void     U_delay    (uint16_t millis);

#ifdef __amigaos__
bool U_request (struct Window *win, char *posTxt, char *negTxt, char* format, ...);
#endif


void     U_init     (void);
void     U_deinit   (void);

#endif
