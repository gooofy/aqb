#ifndef HAVE_LINK_H
#define HAVE_LINK_H

#include "assem.h"

typedef struct LI_segmentList_      *LI_segmentList;
typedef struct LI_segmentListNode_  *LI_segmentListNode;

struct LI_segmentListNode_
{
    AS_segment         seg;
    LI_segmentListNode next;
};

struct LI_segmentList_
{
    LI_segmentListNode first, last;
};

LI_segmentList LI_SegmentList(void);

void           LI_segmentWriteObjectFile    (AS_object obj, string objfn);

void           LI_segmentListAppend         (LI_segmentList sl, AS_segment seg);
bool           LI_segmentListReadObjectFile (U_poolId pid, LI_segmentList sl, string sourcefn, FILE *f);
bool           LI_link                      (U_poolId pid, LI_segmentList sl);
void           LI_segmentListWriteLoadFile  (LI_segmentList sl, string loadfn);

bool           LI_segmentListReadLoadFile   (U_poolId pid, LI_segmentList sl, string sourcefn, FILE *f);
bool           LI_relocate                  (LI_segmentList sl);

#endif
