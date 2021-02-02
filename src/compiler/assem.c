
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcpy */
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "table.h"
#include "assem.h"
#include "errormsg.h"

AS_instrInfo AS_instrInfoA[AS_NUM_INSTR] = {
    // mn                  isJump hasLabel hasImm hasSrc hasDst srcDnOnly dstDnOnly srcAnOnly dstAnOnly dstIsAlsoSrc, dstIsOnlySrc
    { AS_LABEL,            FALSE, TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_ADD_AnDn_AnDn,    FALSE, FALSE  , FALSE, TRUE , TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ADD_Imm_AnDn,     FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ADD_Imm_sp,       FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_ADDQ_Imm_AnDn,    FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_AND_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_AND_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ASL_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ASL_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ASR_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ASR_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_BEQ,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BNE,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BLT,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BGT,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BLE,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BGE,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BLO,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BHI,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BLS,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BHS,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BRA,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_CMP_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVS_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVS_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVU_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVU_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EOR_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EOR_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EXT_Dn,           FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_LEA_Ofp_An,       FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , FALSE       , FALSE },
    { AS_LINK_fp,          FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_LSL_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_LSL_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_LSR_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_LSR_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_MOVE_AnDn_AnDn,   FALSE, FALSE  , FALSE, TRUE , TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_OAn,     FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , FALSE       , FALSE },
    { AS_MOVE_Imm_RAn,     FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , TRUE        , TRUE  },
    { AS_MOVE_Imm_AnDn,    FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_AnDn_RAn,    FALSE, FALSE  , FALSE, TRUE , TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , TRUE        , TRUE  },
    { AS_MOVE_RAn_AnDn,    FALSE, FALSE  , FALSE, TRUE , TRUE  , FALSE  , FALSE   , TRUE    , FALSE   , FALSE       , FALSE },
    { AS_MOVE_OAn_AnDn,    FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , TRUE    , FALSE   , FALSE       , FALSE },
    { AS_MOVE_AnDn_OAn,    FALSE, FALSE  , FALSE, TRUE , TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , FALSE       , FALSE },
    { AS_MOVE_AnDn_PDsp,   FALSE, FALSE  , FALSE, TRUE , FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_PDsp,    FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_spPI_AnDn,   FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_ILabel_AnDn, FALSE, TRUE   , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Label_AnDn,  FALSE, TRUE   , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Label_Ofp,   FALSE, TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_AnDn_Label,  FALSE, TRUE   , FALSE, TRUE , FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Ofp_AnDn,    FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Ofp_RAn,     FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , TRUE    , FALSE       , FALSE },
    { AS_MOVE_Ofp_Label,   FALSE, TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_AnDn_Ofp,    FALSE, FALSE  , FALSE, TRUE , FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_Ofp,     FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_Label,   FALSE, TRUE   , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_fp_AnDn,     FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVEM_Rs_PDsp,    FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVEM_spPI_Rs,    FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MULS_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_MULS_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_MULU_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_MULU_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_NEG_Dn,           FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_NOT_Dn,           FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_NOP,              FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_OR_Dn_Dn,         FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_OR_Imm_Dn,        FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_JMP,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_JSR_Label,        TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_JSR_An,           FALSE, FALSE  , FALSE, TRUE , FALSE , FALSE  , FALSE   , TRUE    , FALSE   , FALSE       , FALSE },
    { AS_JSR_RAn,          FALSE, FALSE  , FALSE, TRUE , FALSE , FALSE  , FALSE   , TRUE    , FALSE   , FALSE       , FALSE },
    { AS_RTS,              FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_SNE_Dn,           FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_SUB_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_SUB_Imm_AnDn,     FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_SUBQ_Imm_AnDn,    FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_SWAP_Dn,          FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_TST_Dn,           FALSE, FALSE  , FALSE, TRUE , FALSE , TRUE   , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_UNLK_fp,          FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE } 
    };

Temp_temp AS_regs[AS_NUM_REGISTERS];

static Temp_tempSet  g_allRegs, g_dRegs, g_aRegs;
static Temp_tempSet  g_callerSaves, g_calleeSaves;
static S_scope       g_regScope;

static string        g_regnames[AS_NUM_REGISTERS] = { "a0", "a1", "a2", "a3", "a4", "a6", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7" };

bool AS_verifyInstr (AS_instr instr)
{
    if (AS_instrInfoA[instr->mn].hasLabel && !instr->label)
        return FALSE;
    if (!AS_instrInfoA[instr->mn].hasLabel && instr->label)
        return FALSE;

    if (AS_instrInfoA[instr->mn].hasImm && !instr->imm)
        return FALSE;
    if (!AS_instrInfoA[instr->mn].hasImm && instr->imm)
        return FALSE;


    if (AS_instrInfoA[instr->mn].hasSrc && !instr->src)
        return FALSE;
    if (!AS_instrInfoA[instr->mn].hasSrc && instr->src)
        return FALSE;

    if (AS_instrInfoA[instr->mn].hasDst && !instr->dst)
        return FALSE;
    if (!AS_instrInfoA[instr->mn].hasDst && instr->dst)
        return FALSE;

    return TRUE;
}

AS_instr AS_Instr (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

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

AS_instr AS_InstrEx (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

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

AS_instr AS_InstrEx2 (S_pos pos, enum AS_mn mn, enum Temp_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label, Temp_tempSet def, Temp_tempSet use)
{
    AS_instr p = (AS_instr) checked_malloc (sizeof *p);

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
    AS_instrList p = (AS_instrList) checked_malloc (sizeof *p);

    p->first = NULL;
    p->last  = NULL;

    return p;
}

static AS_instrListNode AS_InstrListNode (AS_instr i)
{
    AS_instrListNode n = checked_malloc(sizeof(*n));

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
    AS_instrSet s = checked_malloc(sizeof(*s));

    s->first = NULL;
    s->last  = NULL;

    return s;
}

static AS_instrSetNode AS_InstrSetNode (AS_instr i)
{
    AS_instrSetNode n = checked_malloc(sizeof(*n));

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
            return TRUE;
    }
    return FALSE;
}

bool AS_instrSetAdd (AS_instrSet as, AS_instr i) // returns FALSE if i was already in as, TRUE otherwise
{
    for (AS_instrSetNode n = as->first; n; n=n->next)
    {
        if (n->instr == i)
            return FALSE;
    }

    AS_instrSetNode n = AS_InstrSetNode(i);
    n->prev = as->last;

    if (as->last)
        as->last = as->last->next = n;
    else
        as->first = as->last = n;

    return TRUE;
}

void AS_instrSetAddSet (AS_instrSet as, AS_instrSet as2)
{
    for (AS_instrSetNode n = as2->first; n; n=n->next)
    {
        AS_instrSetAdd (as, n->instr);
    }
}

bool AS_instrSetSub (AS_instrSet as, AS_instr i) // returns FALSE if i was not in as, TRUE otherwise
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

            return TRUE;
        }
    }
    return FALSE;
}

enum Temp_w AS_tySize(Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool:
        case Ty_byte:
        case Ty_ubyte:
            return Temp_w_B;
        case Ty_integer:
        case Ty_uinteger:
            return Temp_w_W;
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_double:
        case Ty_pointer:
        case Ty_forwardPtr:
        case Ty_procPtr:
        case Ty_string:
            return Temp_w_L;
        case Ty_sarray:
        case Ty_darray:
        case Ty_record:
        case Ty_void:
        case Ty_toLoad:
        case Ty_prc:
            assert(0);
    }
    return Temp_w_L;
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
                        case Ty_string:
                        case Ty_pointer:
                            pos += sprintf(&str[pos], "%d", instr->imm->u.i);
                            break;
                        case Ty_single:
                        case Ty_double:
                            switch (dialect)
                            {
                                case AS_dialect_gas:
                                    pos += sprintf(&str[pos], "0x%08x /* %f */", encode_ffp(instr->imm->u.f), instr->imm->u.f);
                                    break;
                                case AS_dialect_ASMPro:
                                    pos += sprintf(&str[pos], "$%08x", encode_ffp(instr->imm->u.f));
                                    break;
                                default:
                                    assert(FALSE);
                            }
                            break;
                        default:
                            EM_error(0, "*** assem.c:instrformat: internal error");
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
                    bool first=TRUE;
                    for (int reg=0; reg<AS_NUM_REGISTERS; reg++)
                    {
                        if (instr->offset & (1<<reg))
                        {
                            string rname = AS_regName (reg);

                            if (first)
                                first = FALSE;
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
        case AS_ADDQ_Imm_AnDn:
            instrformat(str, "    addq`w   #`i, `d", i, dialect);      break;
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
            instrformat(str, "    move`w    `l, `d", i, dialect);      break;
        case AS_MOVE_Label_Ofp:
            instrformat(str, "    move`w    `l, `o(a5)", i, dialect);      break;
        case AS_MOVE_AnDn_Label:
            instrformat(str, "    move`w    `s, `l", i, dialect);      break;
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
        case AS_SUBQ_Imm_AnDn:
            instrformat(str, "    subq`w   #`i, `d", i, dialect);      break;
        case AS_SWAP_Dn:
            instrformat(str, "    swap`w   `d", i, dialect);           break;
        case AS_TST_Dn:
            instrformat(str, "    tst`w    `s", i, dialect);           break;
        case AS_UNLK_fp:
            instrformat(str, "    unlk     a5"   , i, dialect);        break;
        default:
            printf("***internal error: unknown mn %d\n", i->mn);
            assert(0);
    }
}

void AS_printInstrList (FILE *out, AS_instrList iList, AS_dialect dialect)
{
    int line = 0;
    for (AS_instrListNode an = iList->first; an; an=an->next)
    {
        AS_instr instr = an->instr;
        int l = S_getline(instr->pos);
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
#ifdef S_KEEP_SOURCE
                    fprintf (out, "\n    ; L%05d %s\n", l, S_getSourceLine(l));
#else
                    fprintf (out, "\n    ; L%05d\n", l);
#endif
                    break;
                default:
                    assert(FALSE);
            }
            line = l;
        }

        char buf[255];
        AS_sprint(buf, instr, dialect);
        fprintf(out, "%s\n", buf);
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

static bool is8BitConst (Ty_const c)
{
    if (c->ty->kind == Ty_bool)
        return TRUE;

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

void AS_assemble (AS_instrList proc)
{
    // step 0: determine size of segment

    uint32_t seg_size = 0;
    for (AS_instrListNode an = proc->first; an; an=an->next)
    {
        AS_instr instr = an->instr;

        char buf[255];
        AS_sprint(buf, instr, AS_dialect_gas);
        printf("AS_assemble: size of %s\n", buf);

        seg_size += instr_size(instr);

    }



    // for (AS_instrList iList = proc->body; iList; iList=iList->tail)
    // {
    //     AS_instr instr = iList->head;

    //     char buf[255];
    //     AS_sprint(buf, instr, m);
    //     printf("AS_assemble: %s\n", buf);


    //     switch (instr->mn)
    //     {
    //         default:
    //             fprintf (stderr, "*** internal error: unknown mn %d\n", instr->mn);
    //             assert(0);
    //     }
    // }
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
        AS_regs[i] = Temp_NamedTemp (g_regnames[i], Ty_ULong());

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
    S_enter(g_regScope, S_Symbol("a0", TRUE), AS_regs[AS_TEMP_A0]);
    S_enter(g_regScope, S_Symbol("a1", TRUE), AS_regs[AS_TEMP_A1]);
    S_enter(g_regScope, S_Symbol("a2", TRUE), AS_regs[AS_TEMP_A2]);
    S_enter(g_regScope, S_Symbol("a3", TRUE), AS_regs[AS_TEMP_A3]);
    S_enter(g_regScope, S_Symbol("a4", TRUE), AS_regs[AS_TEMP_A4]);
    S_enter(g_regScope, S_Symbol("a6", TRUE), AS_regs[AS_TEMP_A6]);
    S_enter(g_regScope, S_Symbol("d0", TRUE), AS_regs[AS_TEMP_D0]);
    S_enter(g_regScope, S_Symbol("d1", TRUE), AS_regs[AS_TEMP_D1]);
    S_enter(g_regScope, S_Symbol("d2", TRUE), AS_regs[AS_TEMP_D2]);
    S_enter(g_regScope, S_Symbol("d3", TRUE), AS_regs[AS_TEMP_D3]);
    S_enter(g_regScope, S_Symbol("d4", TRUE), AS_regs[AS_TEMP_D4]);
    S_enter(g_regScope, S_Symbol("d5", TRUE), AS_regs[AS_TEMP_D5]);
    S_enter(g_regScope, S_Symbol("d6", TRUE), AS_regs[AS_TEMP_D6]);
    S_enter(g_regScope, S_Symbol("d7", TRUE), AS_regs[AS_TEMP_D7]);
}

