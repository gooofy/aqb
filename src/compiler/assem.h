/*
 * assem.h - m68k assembler definitions
 */

#ifndef ASSEM_H
#define ASSEM_H

enum AS_mn
{                       // Example
    AS_LABEL,           // label:

    AS_ADD_Dn_Dn,       // add.x   d1, d2
    AS_ADD_Imm_Dn,      // add.x   #42, d2
    AS_ADD_Imm_sp,      // add.x   #42, sp

    AS_AND_Dn_Dn,       // and.x  d1, d2
    AS_AND_Imm_Dn,      // and.x  #23, d3

    AS_ASL_Dn_Dn,       // asl.x   d1, d2
    AS_ASL_Imm_Dn,      // asl.x   #42, d2

    AS_ASR_Dn_Dn,       // asr.x   d1, d2
    AS_ASR_Imm_Dn,      // asr.x   #42, d2

    AS_BEQ,             // beq     label
    AS_BNE,             // bne     label
    AS_BLT,             // blt     label
    AS_BGT,             // bgt     label
    AS_BLE,             // ble     label
    AS_BGE,             // bge     label
    AS_BCS,             // bcs     label
    AS_BHI,             // bhi     label
    AS_BLS,             // bls     label
    AS_BCC,             // bcc     label

    AS_CMP_Dn_Dn,       // cmp.x   d0, d7

    AS_DIVS_Dn_Dn,      // divs.x  d1, d2
    AS_DIVS_Imm_Dn,     // divs.x  #23, d3
    AS_DIVU_Dn_Dn,      // divu.x  d1, d2
    AS_DIVU_Imm_Dn,     // divu.x  #23, d3

    AS_EOR_Dn_Dn,       // eor.x  d1, d2
    AS_EOR_Imm_Dn,      // eor.x  #23, d3

    AS_EXT_Dn,          // ext.x   d1

    AS_LINK_fp,         // link    a5, #-4

    AS_LSL_Dn_Dn,       // lsl.x   d1, d2
    AS_LSL_Imm_Dn,      // lsl.x   #42, d2

    AS_LSR_Dn_Dn,       // lsr.x   d1, d2
    AS_LSR_Imm_Dn,      // lsr.x   #42, d2

    AS_MOVE_AnDn_AnDn,  // move.x  d1, d2
    AS_MOVE_Imm_OAn,    // move.x  #23, 42(a6)
    AS_MOVE_Imm_RAn,    // move.x  #23, (a6)
    AS_MOVE_Imm_AnDn,   // move.x  #23, d0
    AS_MOVE_AnDn_RAn,   // move.x  d1, (a6)
    AS_MOVE_RAn_AnDn,   // move.x  (a5), d1
    AS_MOVE_OAn_AnDn,   // move.x  42(a1), d0
    AS_MOVE_AnDn_OAn,   // move.x  d0, 42(a3)
    AS_MOVE_AnDn_PDsp,  // move.x  d1, -(sp)
    AS_MOVE_Imm_PDsp,   // move.x  #23, -(sp)
    AS_MOVE_spPI_AnDn,  // move.x  (sp)+, d1
    AS_MOVE_ILabel_AnDn,// move.x  #label, d1
    AS_MOVE_Label_AnDn, // move.x  label, d6
    AS_MOVE_AnDn_Label, // move.x  d6, label
    AS_MOVE_Ofp_AnDn,   // move.x  42(a5), d0
    AS_MOVE_AnDn_Ofp,   // move.x  d0, 42(a5)
    AS_MOVE_Imm_Ofp,    // move.x  d0, 42(a5)
    AS_MOVE_Imm_Label,  // move.x  #42, label
    AS_MOVE_fp_AnDn,    // move.x  a5, d0

    AS_MULS_Dn_Dn,      // muls.x  d1, d2
    AS_MULS_Imm_Dn,     // muls.x  #42, d2
    AS_MULU_Dn_Dn,      // mulu.x  d1, d2
    AS_MULU_Imm_Dn,     // mulu.x  #42, d2

    AS_NEG_Dn,          // neg.x   d0

    AS_NOT_Dn,          // neg.x   d0

    AS_NOP,             // nop

    AS_OR_Dn_Dn,        // or.x  d1, d2
    AS_OR_Imm_Dn,       // or.x  #42, d2

    AS_JMP,             // jmp     label
    AS_JSR_Label,       // jsr     label
    AS_JSR_An,          // jsr     (a2)
    AS_JSR_RAn,         // jsr     -36(a6)

    AS_RTS,             // rts

    AS_SNE_Dn,          // sne.b   d1

    AS_SUB_Dn_Dn,       // sub.x   d1, d2
    AS_SUB_Imm_Dn,      // sub.x   #42, d2

    AS_SWAP_Dn,         // tst.w   d4

    AS_TST_Dn,          // tst.w   d0

    AS_UNLK_fp          // unlink  a5
};

enum AS_w { AS_w_B, AS_w_W, AS_w_L, AS_w_NONE } ;

typedef struct AS_instr_ *AS_instr;
struct AS_instr_
{
    enum AS_mn     mn;
    enum AS_w      w;
    Temp_label     label;
    Temp_tempList  src, dst;
    Temp_tempLList srcInterf, dstInterf;
    Ty_const       imm;
    short          offset;
};

AS_instr AS_Instr       (enum AS_mn mn, enum AS_w w, Temp_temp src, Temp_temp dst);
AS_instr AS_InstrEx     (enum AS_mn mn, enum AS_w w, Temp_tempList src, Temp_tempList dst, Ty_const imm, long offset, Temp_label label);
AS_instr AS_InstrInterf (enum AS_mn mn, enum AS_w w, Temp_tempList src, Temp_tempList dst, 
                         Temp_tempLList srcInterf, Temp_tempLList dstInterf, Ty_const imm, long offset, Temp_label label);

void     AS_sprint      (string str, AS_instr i, Temp_map m);

typedef struct AS_instrList_ *AS_instrList;
struct AS_instrList_
{
    AS_instr     head;
    AS_instrList tail;
};
AS_instrList AS_InstrList(AS_instr head, AS_instrList tail);

AS_instrList AS_instrUnion(AS_instrList ta, AS_instrList tb);
AS_instrList AS_instrMinus(AS_instrList ta, AS_instrList tb);
AS_instrList AS_instrIntersect(AS_instrList ta, AS_instrList tb);
bool         AS_instrInList(AS_instr i, AS_instrList il);

enum AS_w    AS_tySize(Ty_ty ty);

AS_instrList AS_splice(AS_instrList a, AS_instrList b);

void         AS_printInstrList (FILE *out, AS_instrList iList, Temp_map m);

typedef struct AS_proc_ *AS_proc;
struct AS_proc_
{
  string       prolog;
  AS_instrList body;
  string       epilog;
};

AS_proc AS_Proc(string prolog, AS_instrList body, string epilog);

#endif
