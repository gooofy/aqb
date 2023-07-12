#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcpy */
#include <math.h>

#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "assem.h"
#include "errormsg.h"
#include "logger.h"
#include "options.h"

//#define ENABLE_ASSMBLER_OPT

AS_instrInfo AS_instrInfoA[AS_NUM_INSTR] = {
    // mn                  isJump hasLabel hasImm hasSrc hasDst srcDnOnly dstDnOnly srcAnOnly dstAnOnly dstIsAlsoSrc, dstIsOnlySrc
    { AS_LABEL,            false, true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_ADD_AnDn_AnDn,    false, false  , false, true , true  , false  , false   , false   , false   , true        , false },
    { AS_ADD_Imm_AnDn,     false, false  , true , false, true  , false  , false   , false   , false   , true        , false },
    { AS_ADD_Imm_sp,       false, false  , true , false, false , false  , false   , false   , false   , false       , false },
    { AS_AND_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_AND_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_ASL_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_ASL_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_ASR_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_ASR_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_BEQ,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BNE,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BLT,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BGT,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BLE,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BGE,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BLO,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BHI,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BLS,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BHS,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_BRA,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_CMP_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_DIVS_Dn_Dn,       false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_DIVS_Imm_Dn,      false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_DIVU_Dn_Dn,       false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_DIVU_Imm_Dn,      false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_EOR_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_EOR_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_EXT_Dn,           false, false  , false, false, true  , false  , true    , false   , false   , true        , false },
    { AS_LEA_Ofp_An,       false, false  , false, false, true  , false  , false   , false   , true    , false       , false },
    { AS_LINK_fp,          false, false  , true , false, false , false  , false   , false   , false   , false       , false },
    { AS_LSL_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_LSL_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_LSR_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_LSR_Imm_Dn,       false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_MOVE_AnDn_AnDn,   false, false  , false, true , true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Imm_OAn,     false, false  , true , false, true  , false  , false   , false   , true    , false       , false },
    { AS_MOVE_Imm_RAn,     false, false  , true , false, true  , false  , false   , false   , true    , true        , true  },
    { AS_MOVE_Imm_AnDn,    false, false  , true , false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_AnDn_RAn,    false, false  , false, true , true  , false  , false   , false   , true    , true        , true  },
    { AS_MOVE_RAn_AnDn,    false, false  , false, true , true  , false  , false   , true    , false   , false       , false },
    { AS_MOVE_OAn_AnDn,    false, false  , false, false, true  , false  , false   , true    , false   , false       , false },
    { AS_MOVE_AnDn_OAn,    false, false  , false, true , true  , false  , false   , false   , true    , false       , false },
    { AS_MOVE_AnDn_PDsp,   false, false  , false, true , false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Imm_PDsp,    false, false  , true , false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_spPI_AnDn,   false, false  , false, false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_ILabel_AnDn, false, true   , false, false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Label_AnDn,  false, true   , false, false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Label_Ofp,   false, true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_AnDn_Label,  false, true   , false, true , false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Ofp_AnDn,    false, false  , false, false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Ofp_RAn,     false, false  , false, false, true  , false  , false   , false   , true    , false       , false },
    { AS_MOVE_Ofp_Label,   false, true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_AnDn_Ofp,    false, false  , false, true , false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Imm_Ofp,     false, false  , true , false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_Imm_Label,   false, true   , true , false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVE_fp_AnDn,     false, false  , false, false, true  , false  , false   , false   , false   , false       , false },
    { AS_MOVEM_Rs_PDsp,    false, false  , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_MOVEM_spPI_Rs,    false, false  , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_MULS_Dn_Dn,       false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_MULS_Imm_Dn,      false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_MULU_Dn_Dn,       false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_MULU_Imm_Dn,      false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_NEG_Dn,           false, false  , false, false, true  , false  , true    , false   , false   , true        , false },
    { AS_NOT_Dn,           false, false  , false, false, true  , false  , true    , false   , false   , true        , false },
    { AS_NOP,              false, false  , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_OR_Dn_Dn,         false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_OR_Imm_Dn,        false, false  , true , false, true  , false  , true    , false   , false   , true        , false },
    { AS_JMP,              true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_JSR_Label,        true , true   , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_JSR_An,           false, false  , false, true , false , false  , false   , true    , false   , false       , false },
    { AS_JSR_RAn,          false, false  , false, true , false , false  , false   , true    , false   , false       , false },
    { AS_RTS,              false, false  , false, false, false , false  , false   , false   , false   , false       , false },
    { AS_SNE_Dn,           false, false  , false, false, true  , false  , true    , false   , false   , false       , false },
    { AS_SUB_Dn_Dn,        false, false  , false, true , true  , true   , true    , false   , false   , true        , false },
    { AS_SUB_Imm_AnDn,     false, false  , true , false, true  , false  , false   , false   , false   , true        , false },
    { AS_SWAP_Dn,          false, false  , false, false, true  , false  , true    , false   , false   , true        , false },
    { AS_TST_Dn,           false, false  , false, true , false , true   , false   , false   , false   , false       , false },
    { AS_UNLK_fp,          false, false  , false, false, false , false  , false   , false   , false   , false       , false }
    };

#define SEGLIST_HEADER_SIZE 8       // seglist header: 4 bytes size, 4 bytes ptr to next seg

Temp_temp AS_regs[AS_NUM_REGISTERS];

static Temp_tempSet  g_allRegs, g_dRegs, g_aRegs;
static Temp_tempSet  g_callerSaves, g_calleeSaves;
static S_scope       g_regScope;

static string        g_regnames[AS_NUM_REGISTERS] = { "a0", "a1", "a2", "a3", "a4", "a6", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7" };

bool AS_verifyInstr (AS_instr instr)
{
    if (AS_instrInfoA[instr->mn].hasLabel && !instr->label)
        return false;
    if (!AS_instrInfoA[instr->mn].hasLabel && instr->label)
        return false;

    if (AS_instrInfoA[instr->mn].hasImm && !instr->imm)
        return false;
    if (!AS_instrInfoA[instr->mn].hasImm && instr->imm)
        return false;


    if (AS_instrInfoA[instr->mn].hasSrc && !instr->src)
        return false;
    if (!AS_instrInfoA[instr->mn].hasSrc && instr->src)
        return false;

    if (AS_instrInfoA[instr->mn].hasDst && !instr->dst)
        return false;
    if (!AS_instrInfoA[instr->mn].hasDst && instr->dst)
        return false;

    return true;
}

AS_instr AS_Instr (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst)
{
    AS_instr p = (AS_instr) U_poolAlloc (UP_assem, sizeof *p);

    p->mn     = mn;
    p->w      = w;
    p->src    = src;
    p->dst    = dst;
    p->label  = NULL;
    p->imm    = 0;
    p->offset = 0;

    p->def    = NULL;
    p->use    = NULL;

    p->pos    = pos;

    assert (AS_verifyInstr (p));

    return p;
}

AS_instr AS_InstrEx (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, IR_const imm, long offset, Temp_label label)
{
    AS_instr p = (AS_instr) U_poolAlloc (UP_assem, sizeof *p);

    p->mn     = mn;
    p->w      = w;
    p->src    = src;
    p->dst    = dst;
    p->label  = label;
    p->imm    = imm;
    p->offset = offset;

    p->def    = NULL;
    p->use    = NULL;

    p->pos    = pos;

    assert (AS_verifyInstr (p));

    return p;
}

AS_instr AS_InstrEx2 (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, IR_const imm, long offset, Temp_label label, Temp_tempSet def, Temp_tempSet use)
{
    AS_instr p = (AS_instr) U_poolAlloc (UP_assem, sizeof *p);

    p->mn     = mn;
    p->w      = w;
    p->src    = src;
    p->dst    = dst;
    p->label  = label;
    p->imm    = imm;
    p->offset = offset;

    p->def    = def;
    p->use    = use;

    p->pos    = pos;

    assert (AS_verifyInstr (p));

    return p;
}

AS_instrList AS_InstrList (void)
{
    AS_instrList p = (AS_instrList) U_poolAlloc (UP_assem, sizeof *p);

    p->first = NULL;
    p->last  = NULL;

    return p;
}

static AS_instrListNode AS_InstrListNode (AS_instr i)
{
    AS_instrListNode n = U_poolAlloc (UP_assem, sizeof(*n));

    n->prev  = NULL;
    n->next  = NULL;
    n->instr = i;

    return n;
}

void AS_instrListAppend (AS_instrList al, AS_instr instr)
{
    assert(al);

    AS_instrListNode n = AS_InstrListNode(instr);

    n->prev = al->last;
    if (al->last)
        al->last = al->last->next = n;
    else
        al->first = al->last = n;
}

void AS_instrListPrepend (AS_instrList al, AS_instr instr)
{
    assert(al);

    AS_instrListNode n = AS_InstrListNode(instr);

    n->next = al->first;
    if (al->first)
        al->first = al->first->prev = n;
    else
        al->first = al->last = n;
}

void AS_instrListPrependList (AS_instrList il, AS_instrList il2)
{
    for (AS_instrListNode n=il2->last; n; n=n->prev)
        AS_instrListPrepend (il, n->instr);
}

void AS_instrListAppendList (AS_instrList il, AS_instrList il2)
{
    for (AS_instrListNode n=il2->first; n; n=n->next)
        AS_instrListAppend (il, n->instr);
}

void AS_instrListInsertBefore (AS_instrList al, AS_instrListNode a, AS_instr instr)
{
    assert(al);
    assert(a);

    AS_instrListNode n = AS_InstrListNode(instr);

    n->prev = a->prev;
    a->prev = n;
    n->next = a;

    if (n->prev != NULL)
        n->prev->next = n;
    else
        al->first = n;
}

void AS_instrListInsertAfter (AS_instrList al, AS_instrListNode p, AS_instr instr)
{
    assert(al);
    assert(p);

    AS_instrListNode n = AS_InstrListNode(instr);

    n->next = p->next;
    p->next = n;
    n->prev = p;
    if (n->next)
        n->next->prev = n;
    else
        al->last = n;
}

void AS_instrListRemove (AS_instrList al, AS_instrListNode n)
{
    assert(al);
    assert(n);
    assert(al->first);


    if (n->prev)
    {
        n->prev->next = n->next;
    }
    else
    {
        al->first = n->next;
        if (n->next)
            n->next->prev = NULL;
    }

    if (n->next)
    {
        n->next->prev = n->prev;
    }
    else
    {
        al->last = n->prev;
        if (n->prev)
            n->prev->next = NULL;
    }
}

AS_instrSet AS_InstrSet (void)
{
    AS_instrSet s = U_poolAlloc (UP_assem, sizeof(*s));

    s->first = NULL;
    s->last  = NULL;

    return s;
}

static AS_instrSetNode AS_InstrSetNode (AS_instr i)
{
    AS_instrSetNode n = U_poolAlloc (UP_assem, sizeof(*n));

    n->prev  = NULL;
    n->next  = NULL;
    n->instr = i;

    return n;
}

bool AS_instrSetContains (AS_instrSet as, AS_instr i)
{
    for (AS_instrSetNode n = as->first; n; n=n->next)
    {
        if (n->instr == i)
            return true;
    }
    return false;
}

bool AS_instrSetAdd (AS_instrSet as, AS_instr i) // returns false if i was already in as, true otherwise
{
    for (AS_instrSetNode n = as->first; n; n=n->next)
    {
        if (n->instr == i)
            return false;
    }

    AS_instrSetNode n = AS_InstrSetNode(i);
    n->prev = as->last;

    if (as->last)
        as->last = as->last->next = n;
    else
        as->first = as->last = n;

    return true;
}

void AS_instrSetAddSet (AS_instrSet as, AS_instrSet as2)
{
    for (AS_instrSetNode n = as2->first; n; n=n->next)
    {
        AS_instrSetAdd (as, n->instr);
    }
}

bool AS_instrSetSub (AS_instrSet as, AS_instr i) // returns false if i was not in as, true otherwise
{
    for (AS_instrSetNode n = as->first; n; n=n->next)
    {
        if (n->instr == i)
        {
            if (n->prev)
            {
                n->prev->next = n->next;
            }
            else
            {
                as->first = n->next;
                if (n->next)
                    n->next->prev = NULL;
            }

            if (n->next)
            {
                n->next->prev = n->prev;
            }
            else
            {
                as->last = n->prev;
                if (n->prev)
                    n->prev->next = NULL;
            }

            return true;
        }
    }
    return false;
}

static void instrformat(string str, string strTmpl, AS_instr instr, AS_dialect dialect)
{
    int pos = 0;
    int l   = strlen(strTmpl);

    for (int i=0; i<l; i++)
    {
        if (strTmpl[i] == '`')
        {
            i++;
            switch (strTmpl[i])
            {
                case 'w':
                    str[pos] = '.'; pos++;
                    switch (instr->w)
                    {
                        case Temp_w_B:
                            str[pos] = 'b'; pos++; break;
                        case Temp_w_W:
                            str[pos] = 'w'; pos++; break;
                        case Temp_w_L:
                            str[pos] = 'l'; pos++; break;
                        default:
                            str[pos] = '?'; pos++; break;
                    }
                    break;
                case 's':
                {
                    Temp_snprintf (instr->src, &str[pos], 8);
  	                pos = strlen(str);
                    break;
                }
                case 'd':
                {
                    Temp_snprintf (instr->dst, &str[pos], 8);
  	                pos = strlen(str);
                    break;
                }
                case 'i':
                {
                    switch (instr->imm->ty->kind)
                    {
                        case Ty_bool:
                            pos += sprintf(&str[pos], "%d", instr->imm->u.b ? -1 : 0);
                            break;
                        case Ty_byte:
                        case Ty_ubyte:
                        case Ty_integer:
                        case Ty_uinteger:
                        case Ty_long:
                        case Ty_ulong:
                        case Ty_reference:
                            pos += sprintf(&str[pos], "%d", instr->imm->u.i);
                            break;
                        case Ty_single:
                        case Ty_double:
                            switch (dialect)
                            {
                                case AS_dialect_gas:
                                    pos += sprintf(&str[pos], "0x%08x /* %f */", encode_ffp(instr->imm->u.f), instr->imm->u.f);
                                    break;
                                case AS_dialect_vasm:
                                    pos += sprintf(&str[pos], "0x%08x", encode_ffp(instr->imm->u.f));
                                    break;
                                case AS_dialect_ASMPro:
                                    pos += sprintf(&str[pos], "$%08x", encode_ffp(instr->imm->u.f));
                                    break;
                                default:
                                    assert(false);
                            }
                            break;
                        default:
                            //EM_error({0,0}, "*** assem.c:instrformat: internal error");
                            assert(0);
                    }
                    break;
                }
                case 'o':
                {
                    pos += sprintf(&str[pos], "%d", instr->offset);
                    break;
                }
                case 'l':
                {
                    string s = Temp_labelstring(instr->label);
  	                strcpy(&str[pos], s);
  	                pos += strlen(s);
  	                break;
                }
                case 'R':
                {
                    bool first=true;
                    for (int reg=0; reg<AS_NUM_REGISTERS; reg++)
                    {
                        if (instr->offset & (1<<reg))
                        {
                            string rname = AS_regName (reg);

                            if (first)
                                first = false;
                            else
                                str[pos++] = '/';

                            pos += sprintf(&str[pos], "%s", rname);
                        }
                    }
                    break;
                }
                default:
                    assert(0);
            }
        }
        else
        {
            str[pos] = strTmpl[i];
            pos++;
        }
    }
    str[pos] = 0;
}

void AS_sprint(string str, AS_instr i, AS_dialect dialect)
{
    switch (i->mn)
    {
        case AS_LABEL:           // label:
            sprintf(str, "%s:", Temp_labelstring(i->label));  break;
        case AS_ADD_AnDn_AnDn:
            instrformat(str, "    add`w    `s, `d", i, dialect);       break;
        case AS_ADD_Imm_AnDn:
            instrformat(str, "    add`w    #`i, `d", i, dialect);      break;
        case AS_ADD_Imm_sp:
            instrformat(str, "    add`w    #`i, sp", i, dialect);      break;
        case AS_AND_Dn_Dn:
            instrformat(str, "    and`w    `s, `d", i, dialect);       break;
        case AS_AND_Imm_Dn:
            instrformat(str, "    and`w    #`i, `d", i, dialect);      break;
        case AS_ASL_Dn_Dn:
            instrformat(str, "    asl`w    `s, `d", i, dialect);       break;
        case AS_ASL_Imm_Dn:
            instrformat(str, "    asl`w    #`i, `d", i, dialect);      break;
        case AS_ASR_Dn_Dn:
            instrformat(str, "    asr`w    `s, `d", i, dialect);       break;
        case AS_ASR_Imm_Dn:
            instrformat(str, "    asr`w    #`i, `d", i, dialect);      break;
        case AS_BEQ:
            instrformat(str, "    beq      `l", i, dialect);           break;
        case AS_BNE:
            instrformat(str, "    bne      `l", i, dialect);           break;
        case AS_BLT:
            instrformat(str, "    blt      `l", i, dialect);           break;
        case AS_BGT:
            instrformat(str, "    bgt      `l", i, dialect);           break;
        case AS_BLE:
            instrformat(str, "    ble      `l", i, dialect);           break;
        case AS_BGE:
            instrformat(str, "    bge      `l", i, dialect);           break;
        case AS_BLO:
            instrformat(str, "    blo      `l", i, dialect);           break;
        case AS_BHI:
            instrformat(str, "    bhi      `l", i, dialect);           break;
        case AS_BLS:
            instrformat(str, "    bls      `l", i, dialect);           break;
        case AS_BHS:
            instrformat(str, "    bhs      `l", i, dialect);           break;
        case AS_BRA:
            instrformat(str, "    bra      `l", i, dialect);           break;
        case AS_CMP_Dn_Dn:
            instrformat(str, "    cmp`w    `s, `d", i, dialect);       break;
        case AS_DIVS_Dn_Dn:
            instrformat(str, "    divs`w   `s, `d", i, dialect);       break;
        case AS_DIVS_Imm_Dn:
            instrformat(str, "    divs`w   #`i, `d", i, dialect);      break;
        case AS_DIVU_Dn_Dn:
            instrformat(str, "    divu`w   `s, `d", i, dialect);       break;
        case AS_DIVU_Imm_Dn:
            instrformat(str, "    divu`w   #`i, `d", i, dialect);      break;
        case AS_EOR_Dn_Dn:
            instrformat(str, "    eor`w    `s, `d", i, dialect);       break;
        case AS_EOR_Imm_Dn:
            instrformat(str, "    eor`w    #`i, `d", i, dialect);      break;
        case AS_EXT_Dn:
            instrformat(str, "    ext`w    `d", i, dialect);           break;
        case AS_LEA_Ofp_An:
            instrformat(str, "    lea      `o(a5), `d", i, dialect);   break;
        case AS_LINK_fp:
            instrformat(str, "    link     a5, #`i"    , i, dialect);  break;
        case AS_LSL_Dn_Dn:
            instrformat(str, "    lsl`w    `s, `d", i, dialect);       break;
        case AS_LSL_Imm_Dn:
            instrformat(str, "    lsl`w    #`i, `d", i, dialect);      break;
        case AS_LSR_Dn_Dn:
            instrformat(str, "    lsr`w    `s, `d", i, dialect);       break;
        case AS_LSR_Imm_Dn:
            instrformat(str, "    lsr`w    #`i, `d", i, dialect);      break;
        case AS_MOVE_AnDn_AnDn:
            instrformat(str, "    move`w   `s, `d"   , i, dialect);    break;
        case AS_MOVE_fp_AnDn:
            instrformat(str, "    move`w   a5, `d"   , i, dialect);    break;
        case AS_MOVE_AnDn_PDsp:
            instrformat(str, "    move`w   `s, -(sp)", i, dialect);    break;
        case AS_MOVE_spPI_AnDn:
            instrformat(str, "    move`w   (sp)+, `d", i, dialect);    break;
        case AS_MOVE_Imm_OAn:
            instrformat(str, "    move`w   #`i, `o(`s)", i, dialect);  break;
        case AS_MOVE_Imm_RAn:
            instrformat(str, "    move`w   #`i, (`d)", i, dialect);    break;
        case AS_MOVE_Imm_PDsp:
            instrformat(str, "    move`w   #`i, -(sp)", i, dialect);   break;
        case AS_MOVE_AnDn_RAn:
            instrformat(str, "    move`w   `s, (`d)", i, dialect);     break;
        case AS_MOVE_RAn_AnDn:
            instrformat(str, "    move`w   (`s), `d", i, dialect);     break;
        case AS_MOVE_Imm_AnDn:
            instrformat(str, "    move`w   #`i, `d", i, dialect);      break;
        case AS_MOVE_Ofp_AnDn:  // move.x  42(a5), d0
            instrformat(str, "    move`w   `o(a5), `d", i, dialect);   break;
        case AS_MOVE_Ofp_RAn:   // move.x  42(a5), (a2)
            instrformat(str, "    move`w   `o(a5), (`d)", i, dialect); break;
        case AS_MOVE_Ofp_Label: // move.x  42(a5), label
            instrformat(str, "    move`w   `o(a5), `l", i, dialect);   break;
        case AS_MOVE_AnDn_Ofp:  // move.x  d0, 42(a5)
            instrformat(str, "    move`w   `s, `o(a5)", i, dialect);   break;
        case AS_MOVE_Imm_Ofp:   // move.x  #23, 42(a5)
            instrformat(str, "    move`w   #`i, `o(a5)", i, dialect);  break;
        case AS_MOVE_ILabel_AnDn:
            instrformat(str, "    move`w   #`l, `d", i, dialect);      break;
        case AS_MOVE_Label_AnDn:
            instrformat(str, "    move`w   `l, `d", i, dialect);      break;
        case AS_MOVE_Label_Ofp:
            instrformat(str, "    move`w    `l, `o(a5)", i, dialect);      break;
        case AS_MOVE_AnDn_Label:
            instrformat(str, "    move`w   `s, `l", i, dialect);      break;
        case AS_MOVE_Imm_Label:
            instrformat(str, "    move`w   #`i, `l", i, dialect);      break;
        case AS_MOVEM_Rs_PDsp:
            instrformat(str, "    movem`w  `R, -(sp)", i, dialect);    break;
        case AS_MOVEM_spPI_Rs:
            instrformat(str, "    movem`w  (sp)+, `R", i, dialect);    break;
        case AS_MULS_Dn_Dn:
            instrformat(str, "    muls`w   `s, `d", i, dialect);       break;
        case AS_MULS_Imm_Dn:
            instrformat(str, "    muls`w   #`i, `d", i, dialect);      break;
        case AS_MULU_Dn_Dn:
            instrformat(str, "    mulu`w   `s, `d", i, dialect);       break;
        case AS_MULU_Imm_Dn:
            instrformat(str, "    mulu`w   #`i, `d", i, dialect);      break;
        case AS_NEG_Dn:
            instrformat(str, "    neg`w    `d", i, dialect);           break;
        case AS_NOT_Dn:
            instrformat(str, "    not`w    `d", i, dialect);           break;
        case AS_NOP:
            instrformat(str, "    nop"           , i, dialect);        break;
        case AS_JMP:
            instrformat(str, "    jmp      `l", i, dialect);           break;
        case AS_JSR_Label:
            instrformat(str, "    jsr      `l", i, dialect);           break;
        case AS_JSR_An:
            instrformat(str, "    jsr      (`s)", i, dialect);         break;
        case AS_JSR_RAn:
            instrformat(str, "    jsr      `o(`s)", i, dialect);       break;
        case AS_OR_Dn_Dn:
            instrformat(str, "    or`w     `s, `d", i, dialect);       break;
        case AS_OR_Imm_Dn:
            instrformat(str, "    or`w     #`i, `d", i, dialect);      break;
        case AS_RTS:
            instrformat(str, "    rts"           , i, dialect);        break;
        case AS_SNE_Dn:
            instrformat(str, "    sne      `d", i, dialect);           break;
        case AS_SUB_Dn_Dn:
            instrformat(str, "    sub`w    `s, `d", i, dialect);       break;
        case AS_SUB_Imm_AnDn:
            instrformat(str, "    sub`w    #`i, `d", i, dialect);      break;
        case AS_SWAP_Dn:
            instrformat(str, "    swap`w   `d", i, dialect);           break;
        case AS_TST_Dn:
            instrformat(str, "    tst`w    `s", i, dialect);           break;
        case AS_UNLK_fp:
            instrformat(str, "    unlk     a5"   , i, dialect);        break;
        default:
            LOG_printf (LOG_ERROR, "***internal error: unknown mn %d\n", i->mn);
            assert(0);
    }
}

void AS_printInstrList (FILE *out, AS_instrList iList, AS_dialect dialect)
{
    int line = 0;
    for (AS_instrListNode an = iList->first; an; an=an->next)
    {
        AS_instr instr = an->instr;
        int l = instr->pos.line;
        if (l != line)
        {
            switch (dialect)
            {
                case AS_dialect_gas:
#ifdef S_KEEP_SOURCE
                    fprintf (out, "\n    /* L%05d %s */\n", l, S_getSourceLine(l));
#else
                    fprintf (out, "\n    /* L%05d */\n", l);
#endif
                    break;
                case AS_dialect_ASMPro:
                case AS_dialect_vasm:
#ifdef S_KEEP_SOURCE
                    fprintf (out, "\n    ; L%05d %s\n", l, S_getSourceLine(l));
#else
                    fprintf (out, "\n    ; L%05d\n", l);
#endif
                    break;
                default:
                    assert(false);
            }
            line = l;
        }

        char buf[255];
        AS_sprint(buf, instr, dialect);
        fprintf(out, "%s\n", buf);
    }
}

void AS_logInstrList (AS_instrList iList)
{
    int line = 0;
    for (AS_instrListNode an = iList->first; an; an=an->next)
    {
        AS_instr instr = an->instr;
        int l = instr->pos.line;
        if (l != line)
        {
#ifdef S_KEEP_SOURCE
            LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n    /* L%05d %s */\n", l, S_getSourceLine(l));
#else
            LOG_printf (OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "\n    /* L%05d */\n", l);
#endif
            line = l;
        }

        char buf[255];
        AS_sprint(buf, instr, AS_dialect_gas);
        LOG_printf(OPT_get(OPTION_VERBOSE) ? LOG_INFO : LOG_DEBUG, "%s\n", buf);
    }
}

void AS_printInstrSet (FILE *out, AS_instrSet iSet)
{
    for (AS_instrSetNode an = iSet->first; an; an=an->next)
    {
        char buf[255];
        AS_sprint(buf, an->instr, AS_dialect_gas);
        fprintf(out, "%s\n", buf);
    }
}

/******************************************************************************
 **
 ** machine code generation
 **
 ******************************************************************************/

static uint32_t g_hunk_id = 0;

AS_segment AS_Segment (U_poolId pid, string sourcefn, string name, AS_segKind kind, uint32_t initial_size)
{
    AS_segment seg = U_poolAlloc (pid, sizeof(*seg));

    seg->sourcefn     = sourcefn;
    seg->name         = name;
    seg->kind         = kind;
    seg->hunk_id      = g_hunk_id++;
    seg->mem          = NULL;
    seg->mem_size     = 0;
    seg->mem_pos      = 0;
    seg->relocs       = NULL;
    seg->refs         = NULL;
    seg->defs         = NULL;
    seg->srcMap       = NULL;
    seg->srcMapLast   = NULL;
    seg->frameMap     = NULL;
    seg->frameMapLast = NULL;
    seg->globals      = NULL;

    AS_ensureSegmentSize (pid, seg, initial_size);

    return seg;
}

void AS_segmentAddReloc32 (U_poolId pid, AS_segment seg, AS_segment seg_to, uint32_t off)
{
    if (!seg->relocs)
        seg->relocs = TAB_empty(pid);

    AS_segmentReloc32 prev = TAB_look (seg->relocs, seg_to);

    AS_segmentReloc32 reloc = U_poolAlloc (pid, sizeof(*reloc));

    reloc->offset = off;
    reloc->next   = prev;

    TAB_enter (seg->relocs, seg_to, reloc);
}

void AS_segmentAddRef (U_poolId pid, AS_segment seg, S_symbol sym, uint32_t off, enum Temp_w w)
{
    if (!seg->refs)
        seg->refs = TAB_empty(pid);

    AS_segmentRef ref = U_poolAlloc (pid, sizeof(*ref));

    ref->offset      = off;
    ref->w           = w;
    ref->next        = NULL;

    AS_segmentRef prev = TAB_look (seg->refs, sym);
    if (prev)
        ref->next = prev;

    TAB_enter (seg->refs, sym, ref);
}

void AS_segmentAddDef (U_poolId pid, AS_segment seg, S_symbol sym, uint32_t off, bool absolute)
{
    AS_segmentDef def = U_poolAlloc (pid, sizeof(*def));

    def->sym    = sym;
    def->offset = off;
    def->abs    = absolute;
    def->next   = seg->defs;
    seg->defs   = def;
}

#if 0
static bool is8BitConst (IR_const c)
{
    if (c->ty->kind == Ty_bool)
        return true;

    assert (    ( c->ty->kind == Ty_byte     )
             || ( c->ty->kind == Ty_ubyte    )
             || ( c->ty->kind == Ty_integer  )
             || ( c->ty->kind == Ty_uinteger )
             || ( c->ty->kind == Ty_long     )
             || ( c->ty->kind == Ty_ulong    ) );

    return (c->u.i >=-128) && (c->u.i <= 127);
}

static uint32_t instr_size (AS_instr instr)
{
    switch (instr->mn)
    {
        case AS_LABEL:
            return 0;


        case AS_LINK_fp:         // link    a5, #-4
            return 2;

        case AS_MOVE_Imm_AnDn:   //  36 move.x  #23, d0

            if (AS_isAn (instr->dst))  // movea
            {
                switch (instr->w)
                {
                    case Temp_w_B:
                        fprintf (stderr, "*** internal error: movea.b does not exist.\n");
                        assert(0);
                        return 0;
                    case Temp_w_W:
                        return 2;
                    case Temp_w_L:
                        return 3;
                    default:
                        assert(0);
                        return 0;
                }
            }
            else
            {
                switch (instr->w)
                {
                    case Temp_w_B:
                    case Temp_w_W:
                        return 2;
                    case Temp_w_L:
                        if (is8BitConst (instr->imm))       // moveq
                            return 1;
                        return 3;
                    default:
                        assert(0);
                        return 0;
                }
            }
            break;



        case AS_MOVE_AnDn_PDsp:  //  41 move.x  d1, -(sp)
            return 1;

        // case AS_MOVE_Label_AnDn: //  45 move.x  label, d6


        case AS_JSR_Label:       //  61 jsr     label
            return 3;

        default:
            fprintf (stderr, "*** internal error: unknown mn %d!\n", instr->mn);
            assert(0);
    }
}
#endif

void AS_ensureSegmentSize (U_poolId pid, AS_segment seg, uint32_t min_size)
{
    LOG_printf (LOG_DEBUG, "assem: AS_ensureSegmentSize: id=%d kind=%d mem_size=%d, min_size=%d\n", seg->hunk_id, seg->name, seg->kind, seg->mem_size, min_size);
    uint32_t ms = min_size ? min_size : 64;
    if (!seg->mem || (seg->mem_size < min_size))    // make sure we allocate at least those 8 bytes for seglist chaining
    {
        uint32_t s = seg->mem_size ? seg->mem_size : ms;
        while (s<min_size)
            s = s * 2;

        uint32_t alloc_size = s+SEGLIST_HEADER_SIZE;
        uint8_t *mem = U_poolNonChunkCAlloc (pid, alloc_size);
        mem += SEGLIST_HEADER_SIZE;

        if (!seg->mem)
        {
            seg->mem  = mem;
            LOG_printf (LOG_DEBUG, "assem: allocating segment mem of %zd bytes, alloc_size=%d\n", s, alloc_size);
        }
        else
        {
            memcpy (mem, seg->mem, seg->mem_size);
            seg->mem = mem;
            LOG_printf (LOG_DEBUG, "assem: re-allocating segment mem from %zd bytes to %zd bytes, alloc_size=%d\n", seg->mem_size, s, alloc_size);
        }
        seg->mem_size = s;
    }
    else
    {
        LOG_printf (LOG_DEBUG, "assem: AS_ensureSegmentSize: already big enough.\n");
    }
}

static int32_t getConstInt (IR_const c)
{
    switch (c->ty->kind)
    {
        case Ty_bool:
            return c->u.b ? -1 : 0;
        case Ty_byte:
        case Ty_ubyte:
        case Ty_integer:
        case Ty_uinteger:
        case Ty_long:
        case Ty_ulong:
        case Ty_reference:
            return c->u.i;
        case Ty_single:
        case Ty_double:
        {
            int r = (int) round(c->u.f);
            return r;
        }
        default:
            assert(0);
    }
}

static void emit_u1 (AS_segment seg, uint8_t b)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%02x\n", seg->mem_pos, b);
#endif

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+1);
    uint8_t *p = (uint8_t *) (seg->mem + seg->mem_pos);
    *p = b;
    seg->mem_pos += 1;
}

#if 0
static void emit_i1 (AS_segment seg, int8_t b)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%02x\n", seg->mem_pos, b);
#endif

    AS_ensureSegmentSize (seg, seg->mem_pos+1);
    int8_t *p = (int8_t *) (seg->mem + seg->mem_pos);
    *p = b;
    seg->mem_pos += 1;
}
#endif

static void emit_u2 (AS_segment seg, uint16_t w)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%04x\n", seg->mem_pos, w);
#endif

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+2);
    uint16_t *p = (uint16_t *) (seg->mem + seg->mem_pos);
    *p = ENDIAN_SWAP_16(w);
    seg->mem_pos += 2;
}

static void emit_i2 (AS_segment seg, int16_t w)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%04x\n", seg->mem_pos, w);
#endif

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+2);
    int16_t *p = (int16_t *) (seg->mem + seg->mem_pos);
    *p = ENDIAN_SWAP_16(w);
    seg->mem_pos += 2;
}

static void emit_u4 (AS_segment seg, uint32_t w)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%08x\n", seg->mem_pos, w);
#endif

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+4);
    uint32_t *p = (uint32_t *) (seg->mem + seg->mem_pos);
    *p = ENDIAN_SWAP_32(w);
    seg->mem_pos += 4;
}

static void emit_i4 (AS_segment seg, int32_t w)
{
#if LOG_LEVEL == LOG_DEBUG
    LOG_printf (LOG_DEBUG, "0x%08zx: 0x%08x\n", seg->mem_pos, w);
#endif

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+4);
    int32_t *p = (int32_t *) (seg->mem + seg->mem_pos);
    *p = ENDIAN_SWAP_32(w);
    seg->mem_pos += 4;
}

#if 0
static void emit_str(AS_segment seg, char *str)
{
    uint16_t l = strlen(str);
    emit_u2 (seg, l);
    for (uint16_t i=0; i<l; i++)
        emit_u1 (seg, str[i]);
}
#endif

static uint16_t REG_MASK[AS_NUM_REGISTERS] = {
    0x0100, // a0
    0x0200, // a1
    0x0400, // a2
    0x0800, // a3
    0x1000, // a4
    0x4000, // a6
    0x0001, // d0
    0x0002, // d1
    0x0004, // d2
    0x0008, // d3
    0x0010, // d4
    0x0020, // d5
    0x0040, // d6
    0x0080  // d7
};

static uint16_t REG_MASK_PREDECR[AS_NUM_REGISTERS] = {
    0x0080, // a0
    0x0040, // a1
    0x0020, // a2
    0x0010, // a3
    0x0008, // a4
    0x0002, // a6
    0x8000, // d0
    0x4000, // d1
    0x2000, // d2
    0x1000, // d3
    0x0800, // d4
    0x0400, // d5
    0x0200, // d6
    0x0100  // d7
};



static bool emit_Label (AS_segment seg, TAB_table labels, Temp_label l, bool displacement)
{
    AS_labelInfo li = TAB_look (labels, l);
    if (!li)
    {
        AS_labelFixup fixup = U_poolAlloc (UP_assem, sizeof (*fixup));

        fixup->next         = NULL;
        fixup->seg          = seg;
        fixup->offset       = seg->mem_pos;
        fixup->displacement = displacement;

        li = U_poolAlloc (UP_assem, sizeof (*li));

        li->defined      = false;
        li->seg          = NULL;
        li->offset       = 0;
        li->fixups       = fixup;
        li->label        = l;
        li->ty           = NULL;

        TAB_enter (labels, l, li);

        if (displacement)
            emit_u2 (seg, 0);
        else
            emit_u4 (seg, 0);
    }
    else
    {
        if (li->defined)
        {
            if (displacement)
            {
                int32_t o = li->offset - seg->mem_pos;
                if ((o>32767) || (o<-32768))
                {
                    LOG_printf (LOG_ERROR, "branch offset out of bounds\n");
                    return false;
                }
                emit_i2 (seg, (int16_t) o);
            }
            else
            {
                AS_segmentAddReloc32 (UP_assem, seg, li->seg, seg->mem_pos);
                emit_u4 (seg, li->offset);
            }
        }
        else
        {
            AS_labelFixup fixup = U_poolAlloc (UP_assem, sizeof (*fixup));

            fixup->next         = li->fixups;
            fixup->seg          = seg;
            fixup->offset       = seg->mem_pos;
            fixup->displacement = displacement;

            li->fixups = fixup;

            if (displacement)
                emit_u2 (seg, 0);
            else
                emit_u4 (seg, 0);
        }
    }
    return true;
}

static bool defineLabel (AS_object obj, Temp_label label, AS_segment seg, uint32_t offset, bool expt, IR_type ty)
{
    LOG_printf (LOG_DEBUG, "assem: defineLabel label=%s at seg %d %zd (0x%08zx)\n", S_name(label), seg->hunk_id, offset, offset);
    AS_labelInfo li = TAB_look (obj->labels, label);
    if (li)
    {
        if (li->defined)
        {
            LOG_printf (LOG_ERROR, "error: label %s defined multiple times\n", S_name (label));
            return false;
        }

        // fixup

        AS_segment codeSeg = obj->codeSeg;
        for (AS_labelFixup fu=li->fixups; fu; fu=fu->next)
        {
            LOG_printf (LOG_DEBUG, "assem: FIXUP label=%s at 0x%zx -> %zd\n", S_name(label), fu->offset, offset);
            if (fu->displacement)
            {
                assert (fu->seg == seg);
                uint16_t *p = (uint16_t *) (codeSeg->mem+fu->offset);
                uint16_t o = (uint16_t) (offset-fu->offset);
                *p = ENDIAN_SWAP_16(o);
            }
            else
            {
                uint32_t *p = (uint32_t *) (fu->seg->mem+fu->offset);
                *p = ENDIAN_SWAP_32(offset);
                AS_segmentAddReloc32 (UP_assem, fu->seg, seg, fu->offset);
            }
        }

    }
    else
    {
        li = U_poolAlloc (UP_assem, sizeof (*li));
        TAB_enter (obj->labels, label, li);
    }

    li->defined = true;
    li->seg     = seg;
    li->offset  = offset;
    li->fixups  = NULL;
    li->label   = label;
    li->ty      = ty;

    if (expt)
        AS_segmentAddDef (UP_assem, seg, label, offset, /*absolute=*/false);
    return true;
}

static uint16_t AS_regNumDn (Temp_temp r)
{
    switch (Temp_num(r))
    {
        case AS_TEMP_D0: return 0;
        case AS_TEMP_D1: return 1;
        case AS_TEMP_D2: return 2;
        case AS_TEMP_D3: return 3;
        case AS_TEMP_D4: return 4;
        case AS_TEMP_D5: return 5;
        case AS_TEMP_D6: return 6;
        case AS_TEMP_D7: return 7;
        default:
            assert(false);
    }
    return 0;
}

static uint16_t AS_regNumAn (Temp_temp r)
{
    switch (Temp_num(r))
    {
        case AS_TEMP_A0: return 0;
        case AS_TEMP_A1: return 1;
        case AS_TEMP_A2: return 2;
        case AS_TEMP_A3: return 3;
        case AS_TEMP_A4: return 4;
        case AS_TEMP_A6: return 6;
        default:
            assert(false);
    }
    return 0;
}

static void emit_Imm (AS_segment seg, enum Temp_w w, IR_const imm)
{
    switch (w)
    {
        case Temp_w_B:
        {
            uint16_t c = getConstInt (imm);
            c &= 0xFF;
            emit_u2 (seg, c);
            break;
        }
        case Temp_w_W:
        {
            int16_t c = getConstInt (imm);
            emit_i2 (seg, c);
            break;
        }
        case Temp_w_L:
        {
            switch (imm->ty->kind)
            {
                case Ty_bool:
                case Ty_byte:
                case Ty_ubyte:
                case Ty_integer:
                case Ty_uinteger:
                case Ty_long:
                case Ty_ulong:
                case Ty_reference:
                    emit_i4 (seg, getConstInt (imm));
                    break;
                case Ty_single:
                case Ty_double:
                    emit_u4 (seg, encode_ffp(imm->u.f));
                    break;
                default:
                    //EM_error(0, "*** assem.c:instrformat: internal error");
                    assert(0);
            }
            break;
        }
        default:
            assert(false);
    }
}

static void emit_ADD (AS_segment seg, enum Temp_w w, int regDst, bool dstIsAn, int regSrc, int modeSrc)
{
    uint16_t code = 0xd000;
    if (dstIsAn)
    {
        switch (w)
        {
            case Temp_w_W: code |= (3 << 6) ; break;
            case Temp_w_L: code |= (7 << 6) ; break;
            default: assert(false);
        }
    }
    else
    {
        switch (w)
        {
            case Temp_w_B: break;
            case Temp_w_W: code |= (1 << 6) ; break;
            case Temp_w_L: code |= (2 << 6) ; break;
            default: assert(false);
        }
    }

    code |= regDst  << 9;
    code |= regSrc      ;
    code |= modeSrc << 3;

    emit_u2 (seg, code);
}

#ifdef ENABLE_ASSMBLER_OPT
static void emit_ADDQ (AS_segment seg, enum Temp_w w, uint16_t c, uint16_t mode, uint16_t reg)
{
    //   ADDQ.L  #4,A7           ;003c: 588f
    c = c == 8 ? 0 : c;
    uint16_t code = 0x5000 | (c<<9) | (mode<<3) | reg;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= 0x0040; break;
        case Temp_w_L: code |= 0x0080; break;
        default: assert(false);
    }
    emit_u2(seg, code);
}
#endif

static void emit_ASL_ASR (AS_segment seg, enum Temp_w w, uint16_t cnt_reg, uint16_t dr, uint16_t i_r, uint16_t reg)
{
    uint16_t code = 0xe000;
    switch (w)
    {
        case Temp_w_B: code |= (0 << 6); break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }
    code |= cnt_reg << 9;
    code |= dr << 8;
    code |= i_r << 5;
    code |= reg;
    emit_u2 (seg, code);
}

static void emit_AND (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0xc000;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= (1 << 6) ; break;
        case Temp_w_L: code |= (2 << 6) ; break;
        default: assert(false);
    }

    code |= regDst  << 9;
    code |= regSrc      ;
    code |= modeSrc << 3;

    emit_u2 (seg, code);
}

static void emit_ANDI (AS_segment seg, enum Temp_w w, uint16_t reg, uint16_t mode)
{
    uint16_t code = 0x0200;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= (1 << 6) ; break;
        case Temp_w_L: code |= (2 << 6) ; break;
        default: assert(false);
    }

    code |= reg      ;
    code |= mode << 3;

    emit_u2 (seg, code);
}

static void emit_JSR (AS_segment seg, uint16_t mode, uint16_t reg)
{
    uint16_t code = 0x4e80;

    code |= mode  << 3;
    code |= reg;

    emit_u2 (seg, code);
}

static void emit_Bcc (AS_segment seg, uint16_t cc)
{
    emit_u2 (seg, 0x6000 | (cc << 8));
}

static void emit_CMP (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0xb000;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }

    code |= regDst  << 9;
    code |= regSrc      ;
    code |= modeSrc << 3;

    emit_u2 (seg, code);
}

static void emit_DIVS (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0x81c0;
    switch (w)
    {
        case Temp_w_W: break;
        default: assert(false);
    }
    code |= regDst  << 9;
    code |= modeSrc << 3;
    code |= regSrc;
    emit_u2 (seg, code);
}

static void emit_DIVU (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0x80c0;
    switch (w)
    {
        case Temp_w_W: break;
        default: assert(false);
    }
    code |= regDst  << 9;
    code |= modeSrc << 3;
    code |= regSrc;
    emit_u2 (seg, code);
}

static void emit_EOR (AS_segment seg, enum Temp_w w, uint16_t regSrc, uint16_t regDst, uint16_t modeDst)
{
    uint16_t code = 0xb000;
    switch (w)
    {
        case Temp_w_B: code |= (4 << 6);break;
        case Temp_w_W: code |= (5 << 6); break;
        case Temp_w_L: code |= (6 << 6); break;
        default: assert(false);
    }

    code |= regSrc << 9;
    code |= modeDst << 3;
    code |= regDst;

    emit_u2 (seg, code);
}

static void emit_EXT (AS_segment seg, enum Temp_w w, uint16_t reg)
{
    uint16_t code = 0x4800;
    switch (w)
    {
        case Temp_w_B: assert(false); break;
        case Temp_w_W: code |= (2 << 6); break;
        case Temp_w_L: code |= (3 << 6); break;
        default: assert(false);
    }

    code |= reg;

    emit_u2 (seg, code);
}

static void emit_LEA (AS_segment seg, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0x41C0;

    code |= regDst  << 9;
    code |= regSrc      ;
    code |= modeSrc << 3;

    emit_u2 (seg, code);
}

static void emit_LSL_LSR (AS_segment seg, enum Temp_w w, uint16_t cnt_reg, uint16_t dr, uint16_t i_r, uint16_t reg)
{
    uint16_t code = 0xe008;
    switch (w)
    {
        case Temp_w_B: code |= (0 << 6); break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }
    code |= cnt_reg << 9;
    code |= dr << 8;
    code |= i_r << 5;
    code |= reg;
    emit_u2 (seg, code);
}

static void emit_MOVE (AS_segment seg, enum Temp_w w, int regDst, int modeDst, int regSrc, int modeSrc)
{
    uint16_t code = 0x0000;
    switch (w)
    {
        case Temp_w_B: code |= 0x1000; break;
        case Temp_w_W: code |= 0x3000; break;
        case Temp_w_L: code |= 0x2000; break;
        default: assert(false);
    }

    code |= regDst  << 9;
    code |= modeDst << 6;
    code |= regSrc      ;
    code |= modeSrc << 3;

    emit_u2 (seg, code);
}

static void emit_MOVEM (AS_segment seg, enum Temp_w w, uint16_t dr, uint16_t mode, uint16_t regs, uint16_t regDst)
{
    // MOVEM.L D2,-(A7)            ;0024: 48e72000
    // MOVEM.L D2-D3,-(A7)         ;0024: 48e73000
    // MOVEM.L D2-D3/A2/A6,-(A7)   ;0108: 48e73022
    // MOVEM.L (A7)+,D2            ;0044: 4cdf0004

    uint16_t code = 0x4880 | (dr<<10) | (mode<<3) | regDst;
    switch (w)
    {
        case Temp_w_W: break;
        case Temp_w_L: code |= 0x0040; break;
        default: assert(false);
    }
    emit_u2 (seg, code);

    uint16_t regmask = 0;
    for (int reg=0; reg<AS_NUM_REGISTERS; reg++)
    {
        if (regs & (1<<reg))
        {
            if (mode==4)
                regmask |= REG_MASK_PREDECR[reg];
            else
                regmask |= REG_MASK[reg];
        }
    }
    emit_u2 (seg, regmask);
}

static void emit_MULS (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0xc1c0;
    switch (w)
    {
        case Temp_w_W: break;
        default: assert(false);
    }
    code |= regDst  << 9;
    code |= modeSrc << 3;
    code |= regSrc;
    emit_u2 (seg, code);
}

static void emit_MULU (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0xc0c0;
    switch (w)
    {
        case Temp_w_W: break;
        default: assert(false);
    }
    code |= regDst  << 9;
    code |= modeSrc << 3;
    code |= regSrc;
    emit_u2 (seg, code);
}

static void emit_NEG (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t modeDst)
{
    uint16_t code = 0x4400;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= 1<<6 ; break;
        case Temp_w_L: code |= 2<<6 ; break;
        default: assert(false);
    }
    code |= regDst;
    code |= modeDst << 3;
    emit_u2 (seg, code);
}

static void emit_NOT (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t modeDst)
{
    uint16_t code = 0x4600;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= 1<<6 ; break;
        case Temp_w_L: code |= 2<<6 ; break;
        default: assert(false);
    }
    code |= regDst;
    code |= modeDst << 3;
    emit_u2 (seg, code);
}

static void emit_OR (AS_segment seg, enum Temp_w w, uint16_t regSrc, uint16_t modeSrc, uint16_t regDst)
{
    uint16_t code = 0x8000;
    switch (w)
    {
        case Temp_w_B: code |= (0 << 6);break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }

    code |= regDst << 9;
    code |= modeSrc << 3;
    code |= regSrc;

    emit_u2 (seg, code);
}

#ifdef ENABLE_ASSMBLER_OPT
static void emit_MOVEQ (AS_segment seg, uint16_t regDst, IR_const imm)
{
    uint8_t c = (uint8_t) getConstInt (imm);
    uint16_t code = 0x7000 | (regDst << 9) | c;
    emit_u2 (seg, code);
}
#endif

static void emit_Scc (AS_segment seg, uint16_t cc, uint16_t reg, uint16_t mode)
{
    emit_u2 (seg, 0x50C0 | (cc << 8) | (mode <<3) | reg);
}

static void emit_SUB (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0x9000;
    switch (w)
    {
        case Temp_w_B: code |= (0 << 6); break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }
    code |= regDst << 9;
    code |= regSrc;
    code |= modeSrc << 3;
    emit_u2 (seg, code);
}

static void emit_SUBA (AS_segment seg, enum Temp_w w, uint16_t regDst, uint16_t regSrc, uint16_t modeSrc)
{
    uint16_t code = 0x9000;
    switch (w)
    {
        case Temp_w_B: assert(false); break;
        case Temp_w_W: code |= (3 << 6); break;
        case Temp_w_L: code |= (7 << 6); break;
        default: assert(false);
    }
    code |= regDst << 9;
    code |= regSrc;
    code |= modeSrc << 3;
    emit_u2 (seg, code);
}

static void emit_SUBQ (AS_segment seg, enum Temp_w w, uint16_t data, uint16_t reg, uint16_t mode)
{
    uint16_t code = 0x5100;
    switch (w)
    {
        case Temp_w_B: code |= (0 << 6); break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }
    code |= data << 9;
    code |= mode << 3;
    code |= reg;
    emit_u2 (seg, code);
}

static void emit_SWAP (AS_segment seg, uint16_t reg)
{
    uint16_t code = 0x4840 | reg;
    emit_u2 (seg, code);
}

static void emit_TST (AS_segment seg, enum Temp_w w, uint16_t reg, uint16_t mode)
{
    uint16_t code = 0x4a00 | reg;
    switch (w)
    {
        case Temp_w_B: break;
        case Temp_w_W: code |= (1 << 6); break;
        case Temp_w_L: code |= (2 << 6); break;
        default: assert(false);
    }
    emit_u2 (seg, code);
}

static void emit_UNLK (AS_segment seg, uint16_t reg)
{
    // UNLK    A5          ;0048: 4e5d
    uint16_t code = 0x4e58 | reg;
    emit_u2 (seg, code);
}

AS_object AS_Object (string sourcefn, string name)
{
    AS_object obj = U_poolAlloc (UP_assem, sizeof(*obj));

    obj->labels   = TAB_empty(UP_assem);
    obj->codeSeg  = AS_Segment (UP_assem, sourcefn, strprintf (UP_assem, ".text"), AS_codeSeg, AS_INITIAL_CODE_SEGMENT_SIZE);
    obj->dataSeg  = AS_Segment (UP_assem, sourcefn, strprintf (UP_assem, ".data", name), AS_dataSeg, 0);

    return obj;
}

void AS_segmentAddSrcMap (U_poolId pid, AS_segment seg, uint16_t l, uint32_t off)
{
    AS_srcMapNode mapping = U_poolAlloc (pid, sizeof (*mapping));

    mapping->next   = NULL;
    mapping->line   = l;
    mapping->offset = off;

    if (seg->srcMapLast)
        seg->srcMapLast = seg->srcMapLast->next = mapping;
    else
        seg->srcMap = seg->srcMapLast = mapping;
}

AS_frameMapNode AS_segmentAddFrameMap (U_poolId pid, AS_segment seg, Temp_label label, uint32_t code_start, uint32_t code_end)
{
    AS_frameMapNode fmn = U_poolAlloc (pid, sizeof (*fmn));

    fmn->next       = NULL;
    fmn->label      = label;
    fmn->code_start = code_start;
    fmn->code_end   = code_end;
    fmn->vars       = NULL;

    if (seg->frameMapLast)
        seg->frameMapLast = seg->frameMapLast->next = fmn;
    else
        seg->frameMap = seg->frameMapLast = fmn;

    return fmn;
}

void AS_frameMapAddFVI (U_poolId pid, AS_frameMapNode fmn, S_symbol sym, IR_type ty, int offset)
{
    AS_frameVarNode fvn = U_poolAlloc (pid, sizeof (*fvn));

    fvn->next       = fmn->vars;
    fvn->sym        = sym;
    fvn->ty         = ty;
    fvn->offset     = offset;

    fmn->vars = fvn;
}

void AS_segmentAddGVI (U_poolId pid, AS_segment seg, Temp_label label, IR_type ty, uint32_t offset)
{
    LOG_printf(LOG_DEBUG, "assem: AS_segmentAddGVI label=%s offset=%d ty=0x%08lx\n", S_name(label), offset, ty);
    //U_delay(1000);

    AS_globalVarNode gvn = U_poolAlloc (pid, sizeof (*gvn));

    gvn->next       = seg->globals;
    gvn->label      = label;
    gvn->ty         = ty;
    gvn->seg        = seg;
    gvn->offset     = offset;

    seg->globals = gvn;
}

bool AS_assembleCode (AS_object obj, AS_instrList il, bool expt, CG_frame frame)
{
    AS_segment seg = obj->codeSeg;

    uint32_t code_start = seg->mem_pos;

    bool first_label = true;
    int cur_line = -1;
    for (AS_instrListNode an = il->first; an; an=an->next)
    {
        AS_instr instr = an->instr;

        char buf[255];
        AS_sprint(buf, instr, AS_dialect_gas);
        LOG_printf(LOG_DEBUG, "assem: AS_assembleCode: (mn=%3d) %s\n", instr->mn, buf);

        if (OPT_get (OPTION_DEBUG))
        {
            int l = instr->pos.line;
            if (l>cur_line)
            {
                AS_segmentAddSrcMap(UP_assem, seg, l, seg->mem_pos);
                cur_line = l;
            }
        }

        switch (instr->mn)
        {
            case AS_LABEL:
            {
                if (!defineLabel (obj, instr->label, seg, seg->mem_pos, expt && first_label, /*ty=*/NULL))
                    return false;
                first_label = false;
                break;
            }

            case AS_ADD_AnDn_AnDn:   //   1 add.x   d1, d2
            {
                bool isAnDst = AS_isAn(instr->dst);
                bool isAnSrc = AS_isAn(instr->src);
                emit_ADD(seg, instr->w,
                         /*regDst=*/isAnDst ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst),
                         /*dstIsAn=*/isAnDst,
                         /*regSrc=*/isAnSrc ? AS_regNumAn(instr->src) : AS_regNumDn(instr->src),
                         /*modeSrc=*/isAnSrc ? 1:0);
                break;
            }
            case AS_ADD_Imm_AnDn:    //   2 add.x   #42, d2
            {
                bool isAn = AS_isAn(instr->dst);
#ifdef ENABLE_ASSMBLER_OPT
                int32_t c = getConstInt (instr->imm);
                if ((c>0) && (c<=8))
                {
                    emit_ADDQ (seg, instr->w, c, /*mode=*/isAn ? 1:0, /*reg=*/isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst));
                }
                else
                {
#endif
                    emit_ADD(seg, instr->w, /*regDst=*/isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*dstIsAn=*/isAn, /*regSrc=*/4, /*modeSrc=*/7);
                    emit_Imm (seg, instr->w, instr->imm);
#ifdef ENABLE_ASSMBLER_OPT
                }
#endif
                break;
            }

            case AS_ADD_Imm_sp:      //   3 add.x   #42, sp
            {
#ifdef ENABLE_ASSMBLER_OPT
                int32_t c = getConstInt (instr->imm);
                if ((c>0) && (c<=8))
                {
                    emit_ADDQ (seg, instr->w, c, /*mode=*/1, /*reg=*/7);
                }
                else
                {
#endif
                    emit_ADD(seg, instr->w, /*regDst=*/7, /*dstIsAn=*/true, /*regSrc=*/4, /*modeSrc=*/7);
                    emit_Imm (seg, instr->w, instr->imm);
#ifdef ENABLE_ASSMBLER_OPT
                }
#endif
                break;
            }

            case AS_AND_Dn_Dn:       //   4 and.x  d1, d2
                emit_AND(seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;

            case AS_AND_Imm_Dn:      //   5 andi.x  #23, d3
                emit_ANDI(seg, instr->w, /*reg=*/AS_regNumDn(instr->dst), /*mode=*/0);
                emit_Imm (seg, instr->w, instr->imm);
                break;

            case AS_ASL_Dn_Dn:       //   6 asl.x   d1, d2
            {
                emit_ASL_ASR (seg, instr->w,
                              /*cnt_reg=*/AS_regNumDn(instr->src),
                              /*dr=*/1,
                              /*i_r=*/1,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            }
            case AS_ASL_Imm_Dn:      //   7 asl.x   #42, d2
            {
                int32_t c = getConstInt (instr->imm);
                assert ((c>0)&&(c<=8));
                if (c==8)
                    c = 0;
                emit_ASL_ASR (seg, instr->w,
                              /*cnt_reg=*/c,
                              /*dr=*/1,
                              /*i_r=*/0,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            }
            case AS_ASR_Dn_Dn:       //   8 asr.x   d1, d2
            {
                emit_ASL_ASR (seg, instr->w,
                              /*cnt_reg=*/AS_regNumDn(instr->src),
                              /*dr=*/0,
                              /*i_r=*/1,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            }

            case AS_BEQ: emit_Bcc (seg, /*cc=*/ 7); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BNE: emit_Bcc (seg, /*cc=*/ 6); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BLT: emit_Bcc (seg, /*cc=*/13); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BGT: emit_Bcc (seg, /*cc=*/14); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BLE: emit_Bcc (seg, /*cc=*/15); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BGE: emit_Bcc (seg, /*cc=*/12); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BLO: emit_Bcc (seg, /*cc=*/ 5); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BHI: emit_Bcc (seg, /*cc=*/ 2); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BLS: emit_Bcc (seg, /*cc=*/ 3); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;
            case AS_BHS: emit_Bcc (seg, /*cc=*/ 4); if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true)) return false; break;

            case AS_BRA:                    //  20 bra     label
                emit_u2 (seg, 0x6000);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true))
                    return false;
                break;

            case AS_CMP_Dn_Dn:       //  21 cmp.x   d0, d7
            {
                assert (!AS_isAn(instr->dst)); // FIXME: cmpa
                emit_CMP (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst),
                                         /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            }
            case AS_DIVS_Dn_Dn:      //  22 divs.x  d1, d2
                emit_DIVS (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            case AS_DIVU_Dn_Dn:      //  24 divu.x  d1, d2
                emit_DIVU (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            case AS_EOR_Dn_Dn:       //  26 eor.x  d1, d2
                emit_EOR (seg, instr->w, /*regSrc=*/AS_regNumDn(instr->src), /*regDst=*/AS_regNumDn(instr->dst), /*modeDst=*/0);
                break;
            case AS_EXT_Dn:          //  28 ext.x   d1
                emit_EXT (seg, instr->w, AS_regNumDn(instr->dst));
                break;
            case AS_LEA_Ofp_An:      //  29 lea     24(fp), a1
                emit_LEA (seg, /*regDst=*/AS_regNumAn(instr->dst),
                          /*regSrc=*/5, /*modeSrc=*/5);
                emit_i2 (seg, instr->offset);
                break;

            case AS_LINK_fp:                // 30 LINK.W  A5,#-40         ;0104: 4e55ffd8
                emit_u2 (seg, 0x4e55);
                emit_Imm (seg, instr->w, instr->imm);
                break;

            case AS_MOVEM_Rs_PDsp:
                emit_MOVEM (seg, instr->w, /*dr=*/0, /*mode=*/4, /*regs=*/instr->offset, /*regDst=*/7);
                break;

            case AS_LSL_Dn_Dn:       //  31 lsl.x   d1, d2
                emit_LSL_LSR (seg, instr->w,
                              /*cnt_reg=*/AS_regNumDn(instr->src),
                              /*dr=*/1,
                              /*i_r=*/1,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            case AS_LSL_Imm_Dn:      //  32 lsl.x   #42, d2
            {
                int32_t c = getConstInt (instr->imm);
                assert ((c>0)&&(c<=8));
                if (c==8)
                    c = 0;
                emit_LSL_LSR (seg, instr->w,
                              /*cnt_reg=*/c,
                              /*dr=*/1,
                              /*i_r=*/0,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            }
            case AS_LSR_Dn_Dn:       //  33 lsr.x   d1, d2
                emit_LSL_LSR (seg, instr->w,
                              /*cnt_reg=*/AS_regNumDn(instr->src),
                              /*dr=*/0,
                              /*i_r=*/1,
                              /*reg=*/AS_regNumDn(instr->dst));
                break;
            case AS_MOVE_AnDn_AnDn:          //  35 move.x  d1, d2
            {
                bool isAnDst = AS_isAn(instr->dst);
                bool isAnSrc = AS_isAn(instr->src);
                emit_MOVE (seg, instr->w, /*regDst=*/isAnDst ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/isAnDst ? 1:0,
                                          /*regSrc=*/isAnSrc ? AS_regNumAn(instr->src) : AS_regNumDn(instr->src), /*modeSrc=*/isAnSrc ? 1:0);
                break;
            }
            case AS_MOVE_Imm_RAn:    //  37 move.x  #23, (a6)
                emit_MOVE (seg, instr->w, /*regDst=*/AS_regNumAn(instr->dst), /*modeDst=*/2,
                                          /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                break;

            case AS_MOVE_Imm_AnDn:           //  38 move.x  #23, d0
            {
                bool isAn = AS_isAn(instr->dst);

#ifdef ENABLE_ASSMBLER_OPT
                if ( (instr->w == Temp_w_B) && !isAn )
                {
                    emit_MOVEQ (seg, /*regDst=*/AS_regNumDn(instr->dst), instr->imm);
                }
                else
                {
#endif
                    emit_MOVE (seg, instr->w, /*regDst=*/isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/ isAn ? 1:0,
                                              /*regSrc=*/4, /*modeSrc=*/7);
                    emit_Imm (seg, instr->w, instr->imm);
#ifdef ENABLE_ASSMBLER_OPT
                }
#endif
                break;
            }
            case AS_MOVE_AnDn_RAn:   //  39 move.x  d1, (a6)
            {
                bool isAn = AS_isAn(instr->src);
                emit_MOVE (seg, instr->w, /*regDst=*/AS_regNumAn(instr->dst), /*modeDst=*/2,
                                          /*regSrc=*/isAn ? AS_regNumAn(instr->src):AS_regNumDn(instr->src), /*modeSrc=*/isAn?1:0);
                break;
            }
            case AS_MOVE_RAn_AnDn:   //  40 move.x  (a5), d1
            {
                bool isAn = AS_isAn(instr->dst);
                emit_MOVE (seg, instr->w, /*regDst=*/isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/isAn ? 1:0,
                                          /*regSrc=*/AS_regNumAn(instr->src), /*modeSrc=*/2);
                break;
            }
            case AS_MOVE_AnDn_PDsp:  // 43 MOVE.L  D2,-(A7)        ;0034: 2f02
            {
                bool isAn = AS_isAn(instr->src);
                emit_MOVE (seg, instr->w, /*regDst=*/7, /*modeDst=*/4,
                                          /*regSrc=*/isAn ? AS_regNumAn(instr->src) : AS_regNumDn(instr->src), /*modeSrc=*/isAn ? 1:0);
                break;
            }

            case AS_MOVE_Imm_PDsp:   //  44 move.x  #23, -(sp)
                emit_MOVE (seg, instr->w, /*regDst=*/7, /*modeDst=*/4,
                                          /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                break;

            case AS_MOVE_ILabel_AnDn:// 46 MOVE.L  #LAB_01A8,D2        ;002e: 243c00003498
            {
                bool isAn = AS_isAn(instr->dst);
                emit_MOVE (seg, instr->w, /*regDst=*/ isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/isAn ? 1:0,
                                          /*regSrc=*/4, /*modeSrc=*/7);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;
            }
            case AS_MOVE_Label_AnDn: //  47 move.x  label, d6
            {
                bool isAn = AS_isAn(instr->dst);
                emit_MOVE (seg, instr->w, /*regDst=*/ isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/isAn ? 1:0,
                                          /*regSrc=*/1, /*modeSrc=*/7);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;
            }
            case AS_MOVE_Label_Ofp:  //  48 move.x  label, 23(a5)
                emit_MOVE (seg, instr->w, /*regDst=*/5, /*modeDst=*/5,
                                          /*regSrc=*/1, /*modeSrc=*/7);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                emit_i2 (seg, instr->offset);
                break;
            case AS_MOVE_AnDn_Label: //  49 move.x  d6, label
            {
                bool isAn = AS_isAn(instr->src);
                emit_MOVE (seg, instr->w, /*regDst=*/1, /*modeDst=*/7,
                                          /*regSrc=*/isAn ? AS_regNumAn(instr->src) : AS_regNumDn(instr->src), /*modeSrc=*/isAn ? 1:0);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;
            }
            case AS_MOVE_Ofp_AnDn:   //  50 move.x  42(a5), d0
            {
                bool isAn = AS_isAn(instr->dst);
                emit_MOVE (seg, instr->w, /*regDst=*/isAn ? AS_regNumAn(instr->dst) : AS_regNumDn(instr->dst), /*modeDst=*/isAn ? 1:0,
                                          /*regSrc=*/5, /*modeSrc=*/5);
                emit_i2 (seg, instr->offset);
                break;
            }
            case AS_MOVE_Ofp_RAn:    //  51 move.x  42(a5), (a0)
                emit_MOVE (seg, instr->w, /*regDst=*/AS_regNumAn(instr->dst), /*modeDst=*/2,
                                          /*regSrc=*/5, /*modeSrc=*/5);
                emit_i2 (seg, instr->offset);
                break;

            case AS_MOVE_Ofp_Label:  //  52 move.x  42(a5), label
                emit_MOVE (seg, instr->w, /*regDst=*/1, /*modeDst=*/7,
                                          /*regSrc=*/5, /*modeSrc=*/5);
                emit_i2 (seg, instr->offset);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;

            case AS_MOVE_AnDn_Ofp:   //  53 move.x  d0, 42(a5)
            {
                bool isAn = AS_isAn(instr->src);
                emit_MOVE (seg, instr->w, /*regDst=*/5, /*modeDst=*/5,
                                          /*regSrc=*/isAn ? AS_regNumAn(instr->src) : AS_regNumDn(instr->src), /*modeSrc=*/isAn ? 1:0);
                emit_i2 (seg, instr->offset);
                break;
            }
            case AS_MOVE_Imm_Ofp:    //  54 move.x  #42, 42(a5)
                emit_MOVE (seg, instr->w, /*regDst=*/5, /*modeDst=*/5,
                                          /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                emit_i2 (seg, instr->offset);
                break;
            case AS_MOVE_Imm_Label:  //  55 move.x  #42, label
                emit_MOVE (seg, instr->w, /*regDst=*/1, /*modeDst=*/7,
                                          /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;
            case AS_MOVEM_spPI_Rs:   // 58
                emit_MOVEM (seg, instr->w, /*dr=*/1, /*mode=*/3, /*regs=*/instr->offset, /*regDst=*/7);
                break;
            case AS_MULS_Dn_Dn:      //  59 muls.x  d1, d2
                emit_MULS (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            case AS_MULS_Imm_Dn:     //  60 muls.x  #42, d2
                emit_MULS (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                break;
            case AS_MULU_Dn_Dn:      //  61 mulu.x  d1, d2
                emit_MULU (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            case AS_MULU_Imm_Dn:     //  62 mulu.x  #42, d2
                emit_MULU (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc=*/4, /*modeSrc=*/7);
                emit_Imm (seg, instr->w, instr->imm);
                break;
            case AS_NEG_Dn:          //  63 neg.x   d0
                emit_NEG (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*modeDst=*/0);
                break;
            case AS_NOT_Dn:          //  64 not.x   d0
                emit_NOT (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*modeDst=*/0);
                break;
            case AS_OR_Dn_Dn:        //  66 or.x  d1, d2
                emit_OR (seg, instr->w, /*regSrc=*/AS_regNumDn(instr->src), /*modeSrc=*/0, /*regDst=*/AS_regNumDn(instr->dst));
                break;
            case AS_JSR_Label:       //  69 jsr     label
            {
#if 0   // FIXME: remove
                int16_t offset;
                if (calcLabelOffset (seg, obj->labels, instr->label, &offset))  // PC-relative jump possible?
                {
                    emit_JSR (seg, /*mode=*/7, /*reg=*/2);
                    emit_i2 (seg, offset);
                }
                else
                {
                    emit_JSR (seg, /*mode=*/7, /*reg=*/1);
                    if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                        return false;
                }
#endif
#if 0   // FIXME: remove
                if (!isLabelExternal (seg, obj->labels, instr->label))  // PC-relative jump possible?
                {
                    emit_JSR (seg, /*mode=*/7, /*reg=*/2);
                    if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/true))
                        return false;
                }
                else
                {
                    emit_JSR (seg, /*mode=*/7, /*reg=*/1);
                    if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                        return false;
                }
#endif
                emit_JSR (seg, /*mode=*/7, /*reg=*/1);
                if (!emit_Label (seg, obj->labels, instr->label, /*displacement=*/false))
                    return false;
                break;
            }
            case AS_JSR_An:          //  70 jsr     (a2)
                emit_JSR (seg, /*mode=*/2, /*reg=*/AS_regNumAn(instr->src));
                break;
            case AS_JSR_RAn:         //  71 jsr     -36(a6)
                emit_JSR (seg, /*mode=*/5, /*reg=*/AS_regNumAn(instr->src));
                emit_i2 (seg, instr->offset);
                break;
            case AS_RTS:             //  72
                emit_u2 (seg, 0x4e75);
                break;
            case AS_SNE_Dn:          //  73 sne.b   d1
                emit_Scc (seg, /*cc=*/ 6, /*reg=*/AS_regNumDn(instr->dst), /*mode=*/0);
                break;
            case AS_SUB_Dn_Dn:       //  74 sub.x   d1, d2
                emit_SUB (seg, instr->w, /*regDst=*/AS_regNumDn(instr->dst), /*regSrc*/AS_regNumDn(instr->src), /*modeSrc=*/0);
                break;
            case AS_SUB_Imm_AnDn:    //  75 sub.x   #42, d2
            {
                bool isAnDst = AS_isAn(instr->dst);
                if (isAnDst)
                {
                    emit_SUBA (seg, instr->w, /*regDst=*/ AS_regNumAn(instr->dst), /*regSrc*/4, /*modeSrc=*/7);
                    emit_Imm (seg, instr->w, instr->imm);
                }
                else
                {
                    int32_t c = getConstInt (instr->imm);
                    if ((c>=1) && (c<=8))
                    {
                        if (c==8)
                            c = 0;
                        emit_SUBQ (seg, instr->w, /*data=*/c, /*reg=*/ AS_regNumDn(instr->dst), /*mode=*/0);
                    }
                    else
                    {
                        emit_SUB (seg, instr->w, /*regDst=*/ AS_regNumDn(instr->dst), /*regSrc*/4, /*modeSrc=*/7);
                        emit_Imm (seg, instr->w, instr->imm);
                    }
                }
                break;
            }
            case AS_SWAP_Dn:         //  76 swap     d4
                emit_SWAP (seg, /*reg=*/ AS_regNumDn(instr->dst));
                break;

            case AS_TST_Dn:          //  77 tst.x   d0
                emit_TST (seg, instr->w, /*reg=*/ AS_regNumDn(instr->src), /*mode=*/0);
                break;

            case AS_UNLK_fp:         //  78 unlink  a5
                emit_UNLK (seg, /*reg=*/5);
                break;

            default:
                LOG_printf (LOG_ERROR, "assem: unknown mn %d\n", instr->mn);
                assert(false);
        }
    }


    if (OPT_get (OPTION_DEBUG))
    {
        LOG_printf (LOG_DEBUG, "assem: adding frame debug info for frame %s ... \n", S_name (frame->name));
        AS_frameMapNode fmn = AS_segmentAddFrameMap(UP_assem, seg, frame->name, code_start, seg->mem_pos);

        for (CG_frameVarInfo fvi = frame->vars; fvi; fvi=fvi->next)
        {
            if (!fvi->sym)  // anonymous/temp variable
                continue;
            LOG_printf (LOG_DEBUG, "assem: adding frame var info fvi=0x%08lx\n", fvi);
            LOG_printf (LOG_DEBUG, "assem: frame debug info: %s\n", S_name (fvi->sym));
            AS_frameMapAddFVI (UP_assem, fmn, fvi->sym, fvi->ty, fvi->offset);
        }
    }

    return true;
}

bool AS_assembleString (AS_object obj, Temp_label label, string str, uint32_t msize)
{
    AS_segment seg = obj->dataSeg;

    if (!defineLabel (obj, label, seg, seg->mem_pos, /*expt=*/ false, /*ty=*/NULL))
        return false;

    AS_ensureSegmentSize (UP_assem, seg, seg->mem_pos+msize);

    memcpy (seg->mem+seg->mem_pos, str, msize);

    seg->mem_pos += msize;

    return true;
}

void AS_assembleDataAlign2 (AS_object o)
{
    AS_segment seg = o->dataSeg;
    if (seg->mem_pos % 2)
        emit_u1 (seg, 0x0);
}

bool AS_assembleDataLabel (AS_object o, Temp_label label, bool expt, IR_type ty)
{
    if (!defineLabel (o, label, o->dataSeg, o->dataSeg->mem_pos, expt, ty))
        return false;
    return true;
}

void AS_assembleDataFill (AS_segment seg, uint32_t size)
{
    uint32_t done = 0;
    while ((done+4)<=size)
    {
        emit_u4(seg, 0);
        done += 4;
    }
    uint32_t r = size-done;
    if (r>=2)
    {
        emit_u2(seg, 0);
        done += 2;
    }
    r = size-done;
    if (r)
        emit_u1(seg, 0);
}

void AS_assembleData16 (AS_segment seg, uint16_t data)
{
    emit_u2(seg, data);
}

void AS_assembleData32 (AS_segment seg, uint32_t data)
{
    emit_u4(seg, data);
}

void AS_assembleDataString (AS_segment seg, string data)
{
    for (char *c=data; *c; c++)
        emit_u1(seg, *c);
    emit_u1(seg, 0);
}

void AS_assembleDataPtr (AS_object obj, Temp_label label)
{
    emit_Label (obj->dataSeg, obj->labels, label, /*displacement=*/false);
}

void AS_assembleBSSAlign2 (AS_segment seg)
{
    if (seg->mem_pos % 2)
        seg->mem_pos++;
}

void AS_assembleBSS (AS_segment seg, uint32_t size)
{
    seg->mem_pos += size;
}

void AS_resolveLabels (U_poolId pid, AS_object obj)
{
    LOG_printf(LOG_DEBUG, "assem: AS_resolveLabels\n");

    TAB_iter i = TAB_Iter(obj->labels);

    Temp_label l;
    AS_labelInfo li;
    while (TAB_next (i, (void **)&l, (void **)&li))
    {
        if (!li->defined)
        {
#if LOG_LEVEL == LOG_DEBUG
            LOG_printf(LOG_DEBUG, "AS_resolveLabels: XREF %s\n", S_name (l));
#endif

            for (AS_labelFixup fu=li->fixups; fu; fu=fu->next)
            {
                if (fu->displacement)
                {
                    LOG_printf(LOG_ERROR, "assem: undefined label %s\n", Temp_labelstring(l));
                    continue;
                }

                uint32_t *p = (uint32_t *) (fu->seg->mem+fu->offset);
                *p = 0;
                AS_segmentAddRef (UP_assem, fu->seg, l, fu->offset, Temp_w_L);
            }
        }

        if (OPT_get (OPTION_DEBUG) && li->ty)
        {
            //LOG_printf(LOG_DEBUG, "assem: AS_resolveLabels: AS_segmentAddGVI li->label=0x%08lx ...\n", li->label);
            //U_delay(1000);
            AS_segmentAddGVI (pid, li->seg, li->label, li->ty, li->offset);
        }
    }
    //LOG_printf(LOG_DEBUG, "assem: AS_resolveLabels done\n");
    //U_delay(1000);
}

Temp_tempSet AS_registers (void)
{
    return g_allRegs;
}

Temp_tempSet AS_callersaves (void)
{
    return g_callerSaves;
}

Temp_tempSet AS_calleesaves (void)
{
    return g_calleeSaves;
}

Temp_tempSet AS_aRegs (void)
{
    return g_aRegs;
}

Temp_tempSet AS_dRegs (void)
{
    return g_dRegs;
}

bool AS_isAn (Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=AS_TEMP_A0) && (n<=AS_TEMP_A6);
}

bool AS_isDn (Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=AS_TEMP_D0) && (n<=AS_TEMP_D7);
}

bool AS_isPrecolored (Temp_temp reg)
{
    int n = Temp_num(reg);
    return (n>=0) && (n<AS_NUM_REGISTERS);
}

Temp_temp AS_lookupReg (S_symbol sym)
{
    return (Temp_temp) S_look(g_regScope, sym);
}

string AS_regName (int reg)
{
    return g_regnames[reg];
}

void AS_init (void)
{
    for (int i=0; i<AS_NUM_REGISTERS; i++)
        AS_regs[i] = Temp_NamedTemp (g_regnames[i], Temp_w_L);

    g_allRegs = NULL;
    g_dRegs   = NULL;
    g_aRegs   = NULL;
    bool bAdded;

    for (int i = AS_TEMP_A0; i<=AS_TEMP_D7; i++)
    {
        Temp_temp t = AS_regs[i];
        assert (Temp_num(t)==i);
        g_allRegs = Temp_tempSetAdd (g_allRegs, t, &bAdded);
        assert (bAdded);
        if (AS_isAn(t))
        {
            g_aRegs = Temp_tempSetAdd (g_aRegs, t, &bAdded);
            assert (bAdded);
        }
        if (AS_isDn(t))
        {
            g_dRegs = Temp_tempSetAdd (g_dRegs, t, &bAdded);
            assert (bAdded);
        }
    }

    g_callerSaves = NULL;
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, AS_regs[AS_TEMP_D0], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, AS_regs[AS_TEMP_D1], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, AS_regs[AS_TEMP_A0], &bAdded); assert (bAdded);
    g_callerSaves = Temp_tempSetAdd (g_callerSaves, AS_regs[AS_TEMP_A1], &bAdded); assert (bAdded);

    g_calleeSaves = NULL;
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D2], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D3], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D4], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D5], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D6], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_D7], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_A2], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_A3], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_A4], &bAdded); assert (bAdded);
    g_calleeSaves = Temp_tempSetAdd (g_calleeSaves, AS_regs[AS_TEMP_A6], &bAdded); assert (bAdded);

    g_regScope = S_beginScope();
    S_enter(g_regScope, S_Symbol("a0"), AS_regs[AS_TEMP_A0]);
    S_enter(g_regScope, S_Symbol("a1"), AS_regs[AS_TEMP_A1]);
    S_enter(g_regScope, S_Symbol("a2"), AS_regs[AS_TEMP_A2]);
    S_enter(g_regScope, S_Symbol("a3"), AS_regs[AS_TEMP_A3]);
    S_enter(g_regScope, S_Symbol("a4"), AS_regs[AS_TEMP_A4]);
    S_enter(g_regScope, S_Symbol("a6"), AS_regs[AS_TEMP_A6]);
    S_enter(g_regScope, S_Symbol("d0"), AS_regs[AS_TEMP_D0]);
    S_enter(g_regScope, S_Symbol("d1"), AS_regs[AS_TEMP_D1]);
    S_enter(g_regScope, S_Symbol("d2"), AS_regs[AS_TEMP_D2]);
    S_enter(g_regScope, S_Symbol("d3"), AS_regs[AS_TEMP_D3]);
    S_enter(g_regScope, S_Symbol("d4"), AS_regs[AS_TEMP_D4]);
    S_enter(g_regScope, S_Symbol("d5"), AS_regs[AS_TEMP_D5]);
    S_enter(g_regScope, S_Symbol("d6"), AS_regs[AS_TEMP_D6]);
    S_enter(g_regScope, S_Symbol("d7"), AS_regs[AS_TEMP_D7]);
}

