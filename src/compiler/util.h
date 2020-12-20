#ifndef UTIL_H
#define UTIL_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

typedef char *string;
typedef char bool;

#define TRUE  1
#define FALSE 0

void *checked_malloc (size_t size);
void *checked_calloc (size_t nmemb, size_t len);
void U_memstat(void);

string String(const char *); // allocs mem + copies string

string strconcat(const char *s1, const char*s2); // allocates mem for concatenated string

string strprintf(const char *format, ...); // allocates mem for resulting string
string strlower(const char *s);  // allocates mem for resulting string

int strcicmp(string a, string b); // string ignore case compare

void   strserialize(FILE *out, string str);
string strdeserialize(FILE *in);

/* FFP - Motorola Fast Floating Point format support */

uint32_t encode_ffp(float        f);
float    decode_ffp(uint32_t fl);

void U_init (size_t mempool_size);

#endif
