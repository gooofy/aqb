#ifndef HAVE_CODEGEN_H
#define HAVE_CODEGEN_H

#include "scanner.h"
#include "types.h"
#include "hashmap.h"
#include "assem.h"

typedef struct CG_item_           CG_item;
typedef struct CG_itemList_      *CG_itemList;
typedef struct CG_itemListNode_  *CG_itemListNode;
typedef struct CG_frame_         *CG_frame;
typedef struct CG_frag_          *CG_frag;
typedef struct CG_dataFragNode_  *CG_dataFragNode;
typedef struct CG_fragList_      *CG_fragList;

typedef enum
{
    IK_none, IK_const, IK_inFrame, IK_inReg, IK_inHeap, IK_cond,
    IK_varPtr
} CG_itemKind;

typedef enum
{
    CG_plus,  CG_minus,  CG_mul, CG_div,
    CG_xor,   CG_eqv,    CG_imp, CG_neg, CG_not, CG_and, CG_or,
    CG_power, CG_intDiv, CG_mod, CG_shl, CG_shr
} CG_binOp;

typedef enum
{
    CG_eq,  CG_ne,  CG_lt,  CG_gt, CG_le, CG_ge
} CG_relOp;

struct CG_item_
{
    CG_itemKind      kind;

    union
    {
        Ty_const                                                       c;
        struct { int offset; Ty_ty ty;                             }   inFrameR;
        Temp_temp                                                      inReg;
        struct { Temp_label l; Ty_ty ty;                           }   inHeap;
        struct { Temp_label l; AS_instrList fixUps; bool postCond; }   condR;
        Temp_temp                                                      varPtr;
    } u;
};

struct CG_itemListNode_
{
    CG_item         item;
    CG_itemListNode next;
};

struct CG_itemList_
{
    CG_itemListNode first, last;
};

struct CG_frame_
{
    Temp_label     name;
    bool           statc;
    map_t          statc_labels;
    CG_itemList    formals;
    bool           globl;           // global frame? FALSE -> local
    int            locals_offset;
};

struct CG_frag_
{
    enum {CG_stringFrag, CG_procFrag, CG_dataFrag} kind;
    union
    {
        struct {Temp_label label; string str;} stringg;
        struct {S_pos pos; Temp_label label; bool expt; AS_instrList body; CG_frame frame;} proc;
        struct {Temp_label label; bool expt; int size; CG_dataFragNode init; CG_dataFragNode initLast;} data;
    } u;
};
struct CG_dataFragNode_
{
    enum {CG_labelNode, CG_constNode} kind;
    union
    {
        Temp_label label;
        Ty_const   c;
    } u;
    CG_dataFragNode next;
};
struct CG_fragList_
{
    CG_frag     head;
    CG_fragList tail;
};

CG_frame        CG_Frame            (S_pos pos, Temp_label name, Ty_formal formals, bool statc);
CG_frame        CG_globalFrame      (void);

void            CG_ConstItem        (CG_item *item, Ty_const c);
void            CG_BoolItem         (CG_item *item, bool b, Ty_ty ty);
void            CG_IntItem          (CG_item *item, int32_t i, Ty_ty ty);
void            CG_UIntItem         (CG_item *item, uint32_t u, Ty_ty ty);
void            CG_FloatItem        (CG_item *item, double f, Ty_ty ty);
void            CG_StringItem       (CG_item *item, string str);
void            CG_HeapPtrItem      (CG_item *item, Temp_label label, Ty_ty ty);
void            CG_ZeroItem         (CG_item *item, Ty_ty ty);
void            CG_OneItem          (CG_item *item, Ty_ty ty);
void            CG_TempItem         (CG_item *item, Ty_ty ty);
void            CG_NoneItem         (CG_item *item);
void            CG_externalVar      (CG_item *item, string name, Ty_ty ty);
void            CG_allocVar         (CG_item *item, CG_frame frame, string name, bool expt, Ty_ty ty);
int             CG_itemOffset       (CG_item *item);
Ty_ty           CG_ty               (CG_item *item);
bool            CG_isVar            (CG_item *item);
bool            CG_isConst          (CG_item *item);
bool            CG_isNone           (CG_item *item);
Ty_const        CG_getConst         (CG_item *item);
int             CG_getConstInt      (CG_item *item);
double          CG_getConstFloat    (CG_item *item);
bool            CG_getConstBool     (CG_item *item);

void            CG_loadVal          (AS_instrList code, S_pos pos, CG_item *item);
void            CG_loadRef          (AS_instrList code, S_pos pos, CG_item *item);
void            CG_loadCond         (AS_instrList code, S_pos pos, CG_item *item);

CG_itemList     CG_ItemList         (void);
CG_itemListNode CG_itemListAppend   (CG_itemList il);

CG_frag         CG_StringFrag       (Temp_label label, string str);
CG_frag         CG_ProcFrag         (S_pos pos, Temp_label label, bool expt, AS_instrList body, CG_frame frame);
CG_frag         CG_DataFrag         (Temp_label label, bool expt, int size);

void            CG_dataFragAddConst (CG_frag dataFrag, Ty_const c);
void            CG_dataFragAddLabel (CG_frag dataFrag, Temp_label label);

CG_fragList     CG_FragList         (CG_frag head, CG_fragList tail);

void            CG_transBinOp       (AS_instrList code, S_pos pos, CG_binOp o, CG_item *left, CG_item *right, Ty_ty ty);
void            CG_transRelOp       (AS_instrList code, S_pos pos, CG_relOp o, CG_item *left, CG_item *right);
void            CG_transJump        (AS_instrList code, S_pos pos, Temp_label l);
void            CG_transLabel       (AS_instrList code, S_pos pos, Temp_label l);
void            CG_transMergeCond   (AS_instrList code, S_pos pos, CG_item *left, CG_item *right);
void            CG_transPostCond    (AS_instrList code, S_pos pos, CG_item *left, bool positive);
#if 0
void            CG_transCJump       (AS_instrList code, S_pos pos, CG_item *test, Temp_label l);
void            CG_transForLoop     (AS_instrList code, S_pos pos, CG_item &loopVar, CG_item &fromItem, CG_item &toItem, CG_item &stepItem);
void            CG_transIf          (S_pos pos, CG_item test, CG_item then, CG_item elsee);
void            CG_transCast        (S_pos pos, CG_item exp, Ty_ty from_ty, Ty_ty to_ty);
void            CG_transWhile       (S_pos pos, CG_item exp, CG_item body, Temp_label exitlbl, Temp_label contlbl);
void            CG_transDo          (S_pos pos, CG_item untilExp, CG_item whileExp, bool condAtEntry, CG_item body, Temp_label exitlbl, Temp_label contlbl);
void            CG_transGoto        (S_pos pos, Temp_label lbl);
void            CG_transGosub       (S_pos pos, Temp_label lbl);
void            CG_transRTS         (S_pos pos);
#endif

void            CG_transAssignment  (AS_instrList code, S_pos pos,  CG_item *left, CG_item *right);
void            CG_transCall        (AS_instrList code, S_pos pos,  Ty_proc proc, CG_itemList args, CG_item *result);
void            CG_transNOP         (AS_instrList code, S_pos pos) ;

void            CG_castItem         (AS_instrList code, S_pos pos, CG_item *item, Ty_ty to_ty);

void            CG_procEntryExit    (S_pos pos, CG_frame frame, AS_instrList body, CG_itemList formals, CG_item *returnVar, Temp_label exitlbl, bool is_main, bool expt);
void            CG_procEntryExitAS  (CG_frag frag);

CG_fragList     CG_getResult        (void);
void            CG_writeASMFile     (FILE *out, CG_fragList frags, AS_dialect dialect);

void            CG_init             (void);

#endif
