#ifndef HAVE_CODEGEN_H
#define HAVE_CODEGEN_H

typedef struct CG_item_           CG_item;
typedef struct CG_itemList_      *CG_itemList;
typedef struct CG_itemListNode_  *CG_itemListNode;
typedef struct CG_frameVarInfo_  *CG_frameVarInfo;
typedef struct CG_frame_         *CG_frame;
typedef struct CG_frag_          *CG_frag;
typedef struct CG_dataFragNode_  *CG_dataFragNode;
typedef struct CG_fragList_      *CG_fragList;

#include "scanner.h"
#include "ir.h"
#include "hashmap.h"
#include "assem.h"

typedef enum
{
    IK_none, IK_const, IK_inFrame, IK_inReg, IK_inHeap, IK_cond,
    IK_varPtr, IK_inFrameRef
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
    IR_type          ty;

    union
    {
        IR_const                                                       c;
        struct { int offset;                                       }   inFrameR;
        Temp_temp                                                      inReg;
        struct { Temp_label l;                                     }   inHeap;
        struct { Temp_label l; AS_instrList fixUps; bool postCond; }   condR;
        Temp_temp                                                      varPtr;
    } u;
};

struct CG_itemListNode_
{
    CG_item         item;
    CG_itemListNode prev, next;
};

struct CG_itemList_
{
    CG_itemListNode first, last;
};

struct CG_frameVarInfo_
{
    CG_frameVarInfo  next;

    S_symbol         sym;
    IR_type            ty;
    int              offset;
    Temp_label       label;
};

struct CG_frame_
{
    Temp_label       name;
    bool             statc;
    map_t            statc_labels;
    CG_itemList      formals;
    bool             globl;           // global frame? FALSE -> local
    int              locals_offset;
    CG_frameVarInfo  vars;
};

struct CG_frag_
{
    enum {CG_stringFrag, CG_procFrag, CG_dataFrag} kind;
    union
    {
        struct {Temp_label label; string str; size_t msize;} stringg;
        struct {S_pos pos; Temp_label label; bool expt; AS_instrList body; CG_frame frame;} proc;
        struct {Temp_label label; bool expt; size_t size; IR_type ty; CG_dataFragNode init; CG_dataFragNode initLast;} data;
    } u;
};
struct CG_dataFragNode_
{
    enum {CG_labelNode, CG_constNode, CG_ptrNode} kind;
    union
    {
        struct { Temp_label label; bool expt; } l; // label  kind
        Temp_label ptr;                            // ptr    kind
        IR_const   c;                              // const  kind
    } u;
    CG_dataFragNode next;
};
struct CG_fragList_
{
    CG_frag     head;
    CG_fragList tail;
};

// RAL: register association list
typedef struct CG_ral_ *CG_ral;
struct CG_ral_
{
    Temp_temp arg;
    Temp_temp reg;
    CG_ral     next;
};

CG_ral          CG_RAL              (Temp_temp arg, Temp_temp reg, CG_ral next);

CG_frame        CG_Frame            (S_pos pos, Temp_label name, IR_formal formals, bool statc);
void            CG_addFrameVarInfo  (CG_frame frame, S_symbol sym, IR_type ty, int offset, Temp_label label);
CG_frag         CG_genGCFrameDesc   (CG_frame frame);
Temp_label      CG_fdTableLabel     (string module_name);

void            CG_ConstItem        (CG_item *item, IR_const c);
void            CG_BoolItem         (CG_item *item, bool b, IR_type ty);
void            CG_IntItem          (CG_item *item, int32_t i, IR_type ty);
void            CG_UIntItem         (CG_item *item, uint32_t u, IR_type ty);
void            CG_FloatItem        (CG_item *item, double f, IR_type ty);
void            CG_StringItem       (AS_instrList code, S_pos pos, CG_item *item, string str);
void            CG_HeapPtrItem      (CG_item *item, Temp_label label, IR_type ty);
void            CG_ZeroItem         (CG_item *item, IR_type ty);
void            CG_OneItem          (CG_item *item, IR_type ty);
void            CG_TempItem         (CG_item *item, IR_type ty);
void            CG_NoneItem         (CG_item *item);
void            CG_externalVar      (CG_item *item, S_symbol name, IR_type ty);
void            CG_allocVar         (CG_item *item, CG_frame frame, S_symbol name, bool expt, IR_type ty);
int             CG_itemOffset       (CG_item *item);
IR_type         CG_ty               (CG_item *item);
enum Temp_w     CG_itemSize         (CG_item *item);
bool            CG_isVar            (CG_item *item);
bool            CG_isConst          (CG_item *item);
bool            CG_isNone           (CG_item *item);
IR_const        CG_getConst         (CG_item *item);
int32_t         CG_getConstInt      (CG_item *item);
double          CG_getConstFloat    (CG_item *item);
bool            CG_getConstBool     (CG_item *item);

void            CG_loadVal          (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item);
void            CG_loadRef          (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item);
void            CG_loadCond         (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item);

CG_itemList     CG_ItemList         (void);
CG_itemListNode CG_itemListAppend   (CG_itemList il);
CG_itemListNode CG_itemListPrepend  (CG_itemList il);

CG_frag         CG_StringFrag       (Temp_label label, string str);
CG_frag         CG_ProcFrag         (S_pos pos, Temp_label label, bool expt, AS_instrList body, CG_frame frame);
CG_frag         CG_DataFrag         (Temp_label label, bool expt, int size, IR_type ty);

void            CG_dataFragAddConst (CG_frag dataFrag, IR_const c);
void            CG_dataFragAddLabel (CG_frag dataFrag, Temp_label label, bool expt);
void            CG_dataFragAddPtr   (CG_frag dataFrag, Temp_label label);
void            CG_dataFragSetPtr   (CG_frag dataFrag, Temp_label label, int idx);

CG_fragList     CG_FragList         (CG_frag head, CG_fragList tail);

void            CG_transBinOp       (AS_instrList code, S_pos pos, CG_frame frame, CG_binOp o, CG_item *left, CG_item *right, IR_type ty);
void            CG_transRelOp       (AS_instrList code, S_pos pos, CG_frame frame, CG_relOp o, CG_item *left, CG_item *right);
void            CG_transIndex       (AS_instrList code, S_pos pos, CG_frame frame, CG_item *array, CG_item *idx);
void            CG_transField       (AS_instrList code, S_pos pos, CG_frame frame, CG_item *recordPtr, IR_member entry);
void            CG_transProperty    (AS_instrList code, S_pos pos, CG_frame frame, CG_item *recordPtr, IR_member entry);
void            CG_transJump        (AS_instrList code, S_pos pos, Temp_label l);
void            CG_transJSR         (AS_instrList code, S_pos pos, Temp_label l);
void            CG_transRTS         (AS_instrList code, S_pos pos);
void            CG_transLabel       (AS_instrList code, S_pos pos, Temp_label l);
void            CG_transMergeCond   (AS_instrList code, S_pos pos, CG_frame frame, CG_item *left, CG_item *right);
void            CG_transPostCond    (AS_instrList code, S_pos pos, CG_item *left, bool positive);
void            CG_transAssignment  (AS_instrList code, S_pos pos, CG_frame frame, CG_item *left, CG_item *right);
void            CG_transCall        (AS_instrList code, S_pos pos, CG_frame frame, IR_proc proc, CG_itemList args, CG_item *result);
void            CG_transCallPtr     (AS_instrList code, S_pos pos, CG_frame frame, IR_proc proc, CG_item *procPtr, CG_itemList args, CG_item *result);
bool            CG_transMethodCall  (AS_instrList code, S_pos pos, CG_frame frame, IR_method method, CG_itemList args, CG_item *result);
void            CG_transNOP         (AS_instrList code, S_pos pos) ;
void            CG_transDeRef       (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item);

void            CG_castItem         (AS_instrList code, S_pos pos, CG_frame frame, CG_item *item, IR_type to_ty);

void            CG_procEntryExit    (S_pos pos, CG_frame frame, AS_instrList body, CG_item *returnVar, Temp_label exitlbl, bool is_main, bool expt);
void            CG_procEntryExitAS  (CG_frag frag);

CG_fragList     CG_getResult        (void);
void            CG_writeASMFile     (FILE *out, CG_fragList frags, AS_dialect dialect);

void            CG_init             (string module_name);

#endif
