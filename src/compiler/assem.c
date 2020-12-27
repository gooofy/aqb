
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcpy */
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "table.h"
#include "assem.h"
#include "frame.h"
#include "errormsg.h"

AS_instrInfo AS_instrInfoA[AS_NUM_INSTR] = {
    // mn                  isJump hasLabel hasImm hasSrc hasDst srcDnOnly dstDnOnly srcAnOnly dstAnOnly dstIsAlsoSrc, dstIsOnlySrc
    { AS_LABEL,            FALSE, TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_ADD_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ADD_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_ADD_Imm_sp,       FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
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
    { AS_BCS,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BHI,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BLS,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_BCC,              TRUE , TRUE   , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_CMP_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVS_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVS_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVU_Dn_Dn,       FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_DIVU_Imm_Dn,      FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EOR_Dn_Dn,        FALSE, FALSE  , FALSE, TRUE , TRUE  , TRUE   , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EOR_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_EXT_Dn,           FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
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
    { AS_MOVE_AnDn_Label,  FALSE, TRUE   , FALSE, TRUE , FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Ofp_AnDn,    FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_AnDn_Ofp,    FALSE, FALSE  , FALSE, TRUE , FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_Ofp,     FALSE, FALSE  , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_Imm_Label,   FALSE, TRUE   , TRUE , FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_MOVE_fp_AnDn,     FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
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
    { AS_SUB_Imm_Dn,       FALSE, FALSE  , TRUE , FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_SWAP_Dn,          FALSE, FALSE  , FALSE, FALSE, TRUE  , FALSE  , TRUE    , FALSE   , FALSE   , TRUE        , FALSE },
    { AS_TST_Dn,           FALSE, FALSE  , FALSE, TRUE , FALSE , TRUE   , FALSE   , FALSE   , FALSE   , FALSE       , FALSE },
    { AS_UNLK_fp,          FALSE, FALSE  , FALSE, FALSE, FALSE , FALSE  , FALSE   , FALSE   , FALSE   , FALSE       , FALSE } 
    };


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

AS_instr AS_Instr (S_pos pos, enum AS_mn mn, enum AS_w w, Temp_temp src, Temp_temp dst)
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

AS_instr AS_InstrEx (S_pos pos, enum AS_mn mn, enum AS_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label)
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

AS_instr AS_InstrEx2 (S_pos pos, enum AS_mn mn, enum AS_w w, Temp_temp src, Temp_temp dst, Ty_const imm, long offset, Temp_label label, Temp_tempSet def, Temp_tempSet use)
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

enum AS_w AS_tySize(Ty_ty ty)
{
    switch (ty->kind)
    {
        case Ty_bool:
        case Ty_byte:
        case Ty_ubyte:
            return AS_w_B;
        case Ty_integer:
        case Ty_uinteger:
            return AS_w_W;
        case Ty_long:
        case Ty_ulong:
        case Ty_single:
        case Ty_double:
        case Ty_varPtr:
        case Ty_pointer:
        case Ty_string:
        case Ty_forwardPtr:
        case Ty_procPtr:
            return AS_w_L;
        case Ty_sarray:
        case Ty_darray:
        case Ty_record:
        case Ty_void:
        case Ty_toLoad:
        case Ty_prc:
            assert(0);
    }
    return AS_w_L;
}

static void instrformat(string str, string strTmpl, AS_instr instr)
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
                        case AS_w_B:
                            str[pos] = 'b'; pos++; break;
                        case AS_w_W:
                            str[pos] = 'w'; pos++; break;
                        case AS_w_L:
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
                            pos += sprintf(&str[pos], "0x%08x /* %f */", encode_ffp(instr->imm->u.f), instr->imm->u.f);
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

void AS_sprint(string str, AS_instr i)
{
    switch (i->mn)
    {
        case AS_LABEL:           // label:
            sprintf(str, "%s:", Temp_labelstring(i->label));     break;
        case AS_ADD_Dn_Dn:
            instrformat(str, "    add`w    `s, `d", i);     break;
        case AS_ADD_Imm_Dn:
            instrformat(str, "    add`w    #`i, `d", i);     break;
        case AS_ADD_Imm_sp:
            instrformat(str, "    add`w    #`i, sp", i);      break;
        case AS_AND_Dn_Dn:
            instrformat(str, "    and`w    `s, `d", i);     break;
        case AS_AND_Imm_Dn:
            instrformat(str, "    and`w    #`i, `d", i);     break;
        case AS_ASL_Dn_Dn:
            instrformat(str, "    asl`w    `s, `d", i);     break;
        case AS_ASL_Imm_Dn:
            instrformat(str, "    asl`w    #`i, `d", i);     break;
        case AS_ASR_Dn_Dn:
            instrformat(str, "    asr`w    `s, `d", i);     break;
        case AS_ASR_Imm_Dn:
            instrformat(str, "    asr`w    #`i, `d", i);     break;
        case AS_BEQ:
            instrformat(str, "    beq      `l", i);           break;
        case AS_BNE:
            instrformat(str, "    bne      `l", i);           break;
        case AS_BLT:
            instrformat(str, "    blt      `l", i);           break;
        case AS_BGT:
            instrformat(str, "    bgt      `l", i);           break;
        case AS_BLE:
            instrformat(str, "    ble      `l", i);           break;
        case AS_BGE:
            instrformat(str, "    bge      `l", i);           break;
        case AS_BCS:
            instrformat(str, "    bcs      `l", i);           break;
        case AS_BHI:
            instrformat(str, "    bhi      `l", i);           break;
        case AS_BLS:
            instrformat(str, "    bls      `l", i);           break;
        case AS_BCC:
            instrformat(str, "    bcc      `l", i);           break;
        case AS_CMP_Dn_Dn:
            instrformat(str, "    cmp`w    `s, `d", i);     break;
        case AS_DIVS_Dn_Dn:
            instrformat(str, "    divs`w   `s, `d", i);     break;
        case AS_DIVS_Imm_Dn:
            instrformat(str, "    divs`w   #`i, `d", i);     break;
        case AS_DIVU_Dn_Dn:
            instrformat(str, "    divu`w   `s, `d", i);     break;
        case AS_DIVU_Imm_Dn:
            instrformat(str, "    divu`w   #`i, `d", i);     break;
        case AS_EOR_Dn_Dn:
            instrformat(str, "    eor`w    `s, `d", i);     break;
        case AS_EOR_Imm_Dn:
            instrformat(str, "    eor`w    #`i, `d", i);     break;
        case AS_EXT_Dn:
            instrformat(str, "    ext`w    `d", i);          break;
        case AS_LINK_fp:
            instrformat(str, "    link     a5, #`i"    , i);  break;
        case AS_LSL_Dn_Dn:
            instrformat(str, "    lsl`w    `s, `d", i);     break;
        case AS_LSL_Imm_Dn:
            instrformat(str, "    lsl`w    #`i, `d", i);     break;
        case AS_LSR_Dn_Dn:
            instrformat(str, "    lsr`w    `s, `d", i);     break;
        case AS_LSR_Imm_Dn:
            instrformat(str, "    lsr`w    #`i, `d", i);     break;
        case AS_MOVE_AnDn_AnDn:
            instrformat(str, "    move`w   `s, `d"   , i);  break;
        case AS_MOVE_fp_AnDn:
            instrformat(str, "    move`w   a5, `d"   , i);   break;
        case AS_MOVE_AnDn_PDsp:
            instrformat(str, "    move`w   `s, -(sp)",  i);  break;
        case AS_MOVE_spPI_AnDn:
            instrformat(str, "    move`w   (sp)+, `d",  i);  break;
        case AS_MOVE_Imm_OAn:
            instrformat(str, "    move`w   #`i, `o(`s)", i); break;
        case AS_MOVE_Imm_RAn:
            instrformat(str, "    move`w   #`i, (`d)", i);   break;
        case AS_MOVE_Imm_PDsp:
            instrformat(str, "    move`w   #`i, -(sp)", i);   break;
        case AS_MOVE_AnDn_RAn:
            instrformat(str, "    move`w   `s, (`d)", i);   break;
        case AS_MOVE_RAn_AnDn:
            instrformat(str, "    move`w   (`s), `d", i);   break;
        case AS_MOVE_Imm_AnDn:
            instrformat(str, "    move`w   #`i, `d", i);     break;
        case AS_MOVE_Ofp_AnDn:  // move.x  42(a5), d0
            instrformat(str, "    move`w   `o(a5), `d", i);  break;
        case AS_MOVE_AnDn_Ofp:  // move.x  d0, 42(a5)
            instrformat(str, "    move`w   `s, `o(a5)", i);  break;
        case AS_MOVE_Imm_Ofp:   // move.x  #23, 42(a5)
            instrformat(str, "    move`w   #`i, `o(a5)", i);  break;
        case AS_MOVE_ILabel_AnDn:
            instrformat(str, "    move`w   #`l, `d", i);     break;
        case AS_MOVE_Label_AnDn:
            instrformat(str, "    move`w    `l, `d", i);     break;
        case AS_MOVE_AnDn_Label:
            instrformat(str, "    move`w    `s, `l", i);     break;
        case AS_MOVE_Imm_Label:
            instrformat(str, "    move`w   #`i, `l", i);      break;
        case AS_MULS_Dn_Dn:
            instrformat(str, "    muls`w   `s, `d", i);     break;
        case AS_MULS_Imm_Dn:
            instrformat(str, "    muls`w   #`i, `d", i);     break;
        case AS_MULU_Dn_Dn:
            instrformat(str, "    mulu`w   `s, `d", i);     break;
        case AS_MULU_Imm_Dn:
            instrformat(str, "    mulu`w   #`i, `d", i);     break;
        case AS_NEG_Dn:
            instrformat(str, "    neg`w    `d", i);          break;
        case AS_NOT_Dn:
            instrformat(str, "    not`w    `d", i);          break;
        case AS_NOP:
            instrformat(str, "    nop"           , i);        break;
        case AS_JMP:
            instrformat(str, "    jmp      `l", i);           break;
        case AS_JSR_Label:
            instrformat(str, "    jsr      `l", i);           break;
        case AS_JSR_An:
            instrformat(str, "    jsr      (`s)", i);        break;
        case AS_JSR_RAn:
            instrformat(str, "    jsr      `o(`s)", i);      break;
        case AS_OR_Dn_Dn:
            instrformat(str, "    or`w     `s, `d", i);     break;
        case AS_OR_Imm_Dn:
            instrformat(str, "    or`w     #`i, `d", i);     break;
        case AS_RTS:
            instrformat(str, "    rts"           , i);        break;
        case AS_SNE_Dn:
            instrformat(str, "    sne      `d", i);          break;
        case AS_SUB_Dn_Dn:
            instrformat(str, "    sub`w    `s, `d", i);     break;
        case AS_SUB_Imm_Dn:
            instrformat(str, "    sub`w    #`i, `d", i);     break;
        case AS_SWAP_Dn:
            instrformat(str, "    swap`w   `d", i);          break;
        case AS_TST_Dn:
            instrformat(str, "    tst`w    `s", i);          break;
        case AS_UNLK_fp:
            instrformat(str, "    unlk     a5"   , i);        break;
        default:
            printf("***internal error: unknown mn %d\n", i->mn);
            assert(0);
    }
}

void AS_printInstrList (FILE *out, AS_instrList iList)
{
    int line = 0;
    for (AS_instrListNode an = iList->first; an; an=an->next)
    {
        AS_instr instr = an->instr;
        int l = S_getline(instr->pos);
        if (l != line)
        {
#ifdef S_KEEP_SOURCE
            fprintf (out, "\n    /* L%05d %s */\n", l, S_getSourceLine(l));
#else
            fprintf (out, "\n    /* L%05d */\n", l);
#endif
            line = l;
        }

        char buf[255];
        AS_sprint(buf, instr);
        fprintf(out, "%s\n", buf);
    }
}

void AS_printInstrSet (FILE *out, AS_instrSet iSet)
{
    for (AS_instrSetNode an = iSet->first; an; an=an->next)
    {
        char buf[255];
        AS_sprint(buf, an->instr);
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

            if (F_isAn (instr->dst))  // movea
            {
                switch (instr->w)
                {
                    case AS_w_B:
                        fprintf (stderr, "*** internal error: movea.b does not exist.\n");
                        assert(0);
                        return 0;
                    case AS_w_W:
                        return 2;
                    case AS_w_L:
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
                    case AS_w_B:
                    case AS_w_W:
                        return 2;
                    case AS_w_L:
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
        AS_sprint(buf, instr);
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

