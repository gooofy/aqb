#ifndef UTIL_H
#define UTIL_H
#include <assert.h>

typedef char *string;
typedef char bool;

#define TRUE 1
#define FALSE 0

void *checked_malloc(int);

string String(const char *); // allocs mem + copies string

string strconcat(const char *s1, const char*s2); // allocates mem for concatenated string

string strprintf(const char *format, ...); // allocates mem for resulting string

/* generic doubly linked list */

typedef struct U_listNode_ *U_listNode;
struct U_listNode_ 
{
    U_listNode prev, next;
};
typedef struct U_list_ *U_list;
struct U_list_ 
{
    U_listNode first, last;
};

U_list U_List(void);
void   U_ListAppend(U_list list, U_listNode node);

/* FFP - Motorola Fast Floating Point format support */

unsigned int encode_ffp(float        f);
float        decode_ffp(unsigned int fl);

#endif
