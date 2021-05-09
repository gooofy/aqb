/*
 * assem.h - m68k assembler interface
 */

#ifndef ASSEM_H
#define ASSEM_H

#include "scanner.h"
#include "temp.h"

#define AS_WORD_SIZE      4
#define AS_NUM_REGISTERS 14

#define AS_TEMP_A0        0
#define AS_TEMP_A1        1
#define AS_TEMP_A2        2
#define AS_TEMP_A3        3
#define AS_TEMP_A4        4
#define AS_TEMP_A6        5
#define AS_TEMP_D0        6
#define AS_TEMP_D1        7
#define AS_TEMP_D2        8
#define AS_TEMP_D3        9
#define AS_TEMP_D4       10
#define AS_TEMP_D5       11
#define AS_TEMP_D6       12
#define AS_TEMP_D7       13

#define INITIAL_CODE_SEGMENT_SIZE   16 * 1024

extern Temp_temp AS_regs[AS_NUM_REGISTERS];

enum AS_mn
{                       //     Example
    AS_LABEL,           //   0 label:

    AS_ADD_AnDn_AnDn,   //   1 add.x   d1, d2
    AS_ADD_Imm_AnDn,    //   2 add.x   #42, d2
    AS_ADD_Imm_sp,      //   3 add.x   #42, sp

    AS_AND_Dn_Dn,       //   4 and.x  d1, d2
    AS_AND_Imm_Dn,      //   5 and.x  #23, d3

    AS_ASL_Dn_Dn,       //   6 asl.x   d1, d2
    AS_ASL_Imm_Dn,      //   7 asl.x   #42, d2

    AS_ASR_Dn_Dn,       //   8 asr.x   d1, d2
    AS_ASR_Imm_Dn,      //   9 asr.x   #42, d2

    AS_BEQ,             //  10 beq     label
    AS_BNE,             //  11 bne     label
    AS_BLT,             //  12 blt     label
    AS_BGT,             //  13 bgt     label
    AS_BLE,             //  14 ble     label
    AS_BGE,             //  15 bge     label
    AS_BLO,             //  16 blo     label
    AS_BHI,             //  17 bhi     label
    AS_BLS,             //  18 bls     label
    AS_BHS,             //  19 bhs     label

    AS_BRA,             //  20 bra     label

    AS_CMP_Dn_Dn,       //  21 cmp.x   d0, d7

    AS_DIVS_Dn_Dn,      //  22 divs.x  d1, d2
    AS_DIVS_Imm_Dn,     //  23 divs.x  #23, d3
    AS_DIVU_Dn_Dn,      //  24 divu.x  d1, d2
    AS_DIVU_Imm_Dn,     //  25 divu.x  #23, d3

    AS_EOR_Dn_Dn,       //  26 eor.x  d1, d2
    AS_EOR_Imm_Dn,      //  27 eor.x  #23, d3

    AS_EXT_Dn,          //  28 ext.x   d1

    AS_LEA_Ofp_An,      //  29 lea     24(fp), a1

    AS_LINK_fp,         //  30 link    a5, #-4

    AS_LSL_Dn_Dn,       //  31 lsl.x   d1, d2
    AS_LSL_Imm_Dn,      //  32 lsl.x   #42, d2

    AS_LSR_Dn_Dn,       //  33 lsr.x   d1, d2
    AS_LSR_Imm_Dn,      //  34 lsr.x   #42, d2

    AS_MOVE_AnDn_AnDn,  //  35 move.x  d1, d2
    AS_MOVE_Imm_OAn,    //  36 move.x  #23, 42(a6)
    AS_MOVE_Imm_RAn,    //  37 move.x  #23, (a6)
    AS_MOVE_Imm_AnDn,   //  38 move.x  #23, d0
    AS_MOVE_AnDn_RAn,   //  39 move.x  d1, (a6)
    AS_MOVE_RAn_AnDn,   //  40 move.x  (a5), d1
    AS_MOVE_OAn_AnDn,   //  41 move.x  42(a1), d0
    AS_MOVE_AnDn_OAn,   //  42 move.x  d0, 42(a3)
    AS_MOVE_AnDn_PDsp,  //  43 move.x  d1, -(sp)
    AS_MOVE_Imm_PDsp,   //  44 move.x  #23, -(sp)
    AS_MOVE_spPI_AnDn,  //  45 move.x  (sp)+, d1
    AS_MOVE_ILabel_AnDn,//  46 move.x  #label, d1
    AS_MOVE_Label_AnDn, //  47 move.x  label, d6
    AS_MOVE_Label_Ofp,  //  48 move.x  label, 23(a5)
    AS_MOVE_AnDn_Label, //  49 move.x  d6, label
    AS_MOVE_Ofp_AnDn,   //  50 move.x  42(a5), d0
    AS_MOVE_Ofp_RAn,    //  51 move.x  42(a5), (a0)
    AS_MOVE_Ofp_Label,  //  52 move.x  42(a5), label
    AS_MOVE_AnDn_Ofp,   //  53 move.x  d0, 42(a5)
    AS_MOVE_Imm_Ofp,    //  54 move.x  #42, 42(a5)
    AS_MOVE_Imm_Label,  //  55 move.x  #42, label
    AS_MOVE_fp_AnDn,    //  56 move.x  a5, d0

    AS_MOVEM_Rs_PDsp,   //  57 movem.x a2-a5,-(sp)
    AS_MOVEM_spPI_Rs,   //  58 movem.x (sp)+, a2-a5

    AS_MULS_Dn_Dn,      //  59 muls.x  d1, d2
    AS_MULS_Imm_Dn,     //  60 muls.x  #42, d2
    AS_MULU_Dn_Dn,      //  61 mulu.x  d1, d2
    AS_MULU_Imm_Dn,     //  62 mulu.x  #42, d2

    AS_NEG_Dn,          //  63 neg.x   d0

    AS_NOT_Dn,          //  64 not.x   d0

    AS_NOP,             //  65 nop

    AS_OR_Dn_Dn,        //  66 or.x  d1, d2
    AS_OR_Imm_Dn,       //  67 or.x  #42, d2

    AS_JMP,             //  68 jmp     label
    AS_JSR_Label,       //  69 jsr     label
    AS_JSR_An,          //  70 jsr     (a2)
    AS_JSR_RAn,         //  71 jsr     -36(a6)

    AS_RTS,             //  72 rts

    AS_SNE_Dn,          //  73 sne.b   d1

    AS_SUB_Dn_Dn,       //  74 sub.x   d1, d2
    AS_SUB_Imm_AnDn,    //  75 sub.x   #42, d2

    AS_SWAP_Dn,         //  76 swap.x   d4

    AS_TST_Dn,          //  77 tst.x   d0

    AS_UNLK_fp,         //  78 unlink  a5

    AS_NUM_INSTR
};

#include "types.h"

typedef struct AS_instr_ *AS_instr;
struct AS_instr_
{
    enum AS_mn     mn;
    enum Temp_w    w;
    Temp_label     label;
    Temp_temp      src, dst;
    Ty_const       imm;
    short          offset;

    Temp_tempSet   def, use; // optional, used in flowgraph.c
    S_pos          pos;      // source code reference
};

AS_instr AS_Instr       (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst);
AS_instr AS_InstrEx     (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label);
AS_instr AS_InstrEx2    (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label, Temp_tempSet def, Temp_tempSet use);

/*
 * AS_instrInfo: general information about 68k instruction set
 */

typedef struct AS_instrInfo_ AS_instrInfo;

struct AS_instrInfo_
{
    enum AS_mn  mn;
    bool        isJump;
    bool        hasLabel;
    bool        hasImm;
    bool        hasSrc;
    bool        hasDst;
    bool        srcDnOnly, dstDnOnly; // TRUE -> src/dst has to be a data register
    bool        srcAnOnly, dstAnOnly; // TRUE -> src/dst has to be a address register
    bool        dstIsAlsoSrc;         // TRUE -> destination register is also a src (i.e. add.x d1, d2)
    bool        dstIsOnlySrc;         // TRUE -> destination register is only a src (i.e. move.x d1, (a0))
};

extern AS_instrInfo AS_instrInfoA[AS_NUM_INSTR]; // array of AS_instrInfos, indexed by mn above

// AS_instrList: mutable, ordered doubly-linked list of assembly instructions

typedef struct AS_instrList_     *AS_instrList;
typedef struct AS_instrListNode_ *AS_instrListNode;

struct AS_instrListNode_
{
    AS_instrListNode next, prev;
    AS_instr         instr;
};

struct AS_instrList_
{
    AS_instrListNode first, last;
};

typedef enum
{
    AS_dialect_gas, AS_dialect_ASMPro
} AS_dialect;

AS_instrList AS_InstrList             (void);
void         AS_instrListAppend       (AS_instrList il, AS_instr instr);
void         AS_instrListPrepend      (AS_instrList il, AS_instr instr);
void         AS_instrListPrependList  (AS_instrList il, AS_instrList il2);
void         AS_instrListAppendList   (AS_instrList il, AS_instrList il2);
void         AS_instrListInsertBefore (AS_instrList il, AS_instrListNode n, AS_instr instr);
void         AS_instrListInsertAfter  (AS_instrList il, AS_instrListNode n, AS_instr instr);
void         AS_instrListRemove       (AS_instrList il, AS_instrListNode n);

// AS_instrSet: mutable set of instrs, still represented as a linked list for speed and iteration

typedef struct AS_instrSet_     *AS_instrSet;
typedef struct AS_instrSetNode_ *AS_instrSetNode;

struct AS_instrSetNode_
{
    AS_instrSetNode next, prev;
    AS_instr        instr;
};

struct AS_instrSet_
{
    AS_instrSetNode first, last;
};

AS_instrSet        AS_InstrSet         (void);
bool               AS_instrSetContains (AS_instrSet as, AS_instr i);
bool               AS_instrSetAdd      (AS_instrSet as, AS_instr i);      // returns FALSE if i was already in as, TRUE otherwise
void               AS_instrSetAddSet   (AS_instrSet as, AS_instrSet as2); // add all elements from as2 to as
bool               AS_instrSetSub      (AS_instrSet as, AS_instr i);      // returns FALSE if i was not in as, TRUE otherwise
static inline bool AS_instrSetIsEmpty  (AS_instrSet as) { return as->first == NULL; }

void               AS_sprint           (string str, AS_instr i, AS_dialect dialect);
void               AS_printInstrList   (FILE *out, AS_instrList iList, AS_dialect dialect);
void               AS_logInstrList     (AS_instrList iList);
void               AS_printInstrSet    (FILE *out, AS_instrSet  is   );

Temp_tempSet       AS_registers        (void);
Temp_tempSet       AS_callersaves      (void);
Temp_tempSet       AS_calleesaves      (void);
Temp_tempSet       AS_aRegs            (void);
Temp_tempSet       AS_dRegs            (void);
bool               AS_isAn             (Temp_temp reg);
bool               AS_isDn             (Temp_temp reg);
bool               AS_isPrecolored     (Temp_temp reg);
Temp_temp          AS_lookupReg        (S_symbol sym);
string             AS_regName          (int reg);

/*
 * 68k machine code generation
 */

#define AS_INITIAL_CODE_SEGMENT_SIZE    16*1024

typedef struct AS_segment_         *AS_segment;
typedef struct AS_segmentReloc32_  *AS_segmentReloc32;
typedef struct AS_segmentRef_      *AS_segmentRef;
typedef struct AS_segmentDef_      *AS_segmentDef;
typedef struct AS_labelInfo_       *AS_labelInfo;
typedef struct AS_object_          *AS_object;

typedef enum {AS_codeSeg, AS_dataSeg, AS_bssSeg, AS_unknownSeg} AS_segKind;
struct AS_segment_
{
    string            sourcefn;
    AS_segKind        kind;
    uint32_t          hunk_id;

    uint8_t          *mem;
    size_t            mem_size;
    size_t            mem_pos;

    TAB_table         relocs;               // AS_segment -> AS_segementReloc32...
    TAB_table         refs;                 // S_symbol -> AS_segmentRef
    AS_segmentDef     defs;
};

struct AS_segmentReloc32_
{
    uint32_t          offset;
    AS_segmentReloc32 next;
};

struct AS_segmentRef_
{
    uint32_t          offset;
    enum Temp_w       w;
    size_t            common_size;
    AS_segmentRef     next;
};

struct AS_segmentDef_
{
    S_symbol          sym;
    uint32_t          offset;
    AS_segmentDef     next;
};

struct AS_labelInfo_
{
    bool              defined;
    bool              displacement;
    AS_segment        seg;
    size_t            offset;   // when not defined (yet) this points to the first fixup chain location, otherwise this is the target segment's offset
};

struct AS_object_
{
    TAB_table         labels;    // label -> AS_labelInfo
    AS_segment        codeSeg;
    AS_segment        dataSeg;
};

AS_segment         AS_Segment            (string sourcefn, AS_segKind kind, size_t initial_size);

void               AS_segmentAddReloc32  (AS_segment seg, AS_segment seg_to, uint32_t off);
void               AS_segmentAddRef      (AS_segment seg, S_symbol sym, uint32_t off, enum Temp_w w, size_t common_size);
void               AS_segmentAddDef      (AS_segment seg, S_symbol sym, uint32_t off);
void               AS_ensureSegmentSize  (AS_segment seg, size_t min_size);

AS_object          AS_Object             (string sourcefn);

bool               AS_assembleCode       (AS_object  o, AS_instrList il, bool expt);
bool               AS_assembleString     (AS_object  o, Temp_label label, string str, size_t msize);
void               AS_assembleDataAlign2 (AS_object  o);
bool               AS_assembleDataLabel  (AS_object  o, Temp_label label, bool expt);
void               AS_assembleDataFill   (AS_segment seg, size_t size);

void               AS_resolveLabels      (AS_object o);

void               AS_init               (void);

#endif
