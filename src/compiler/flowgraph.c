#include <string.h>

#include "flowgraph.h"
#include "errormsg.h"

static FG_graph FG_Graph(void)
{
    FG_graph g = (FG_graph) checked_malloc(sizeof (*g));

    g->nodecount = 0;
    g->nodes     = NULL;
    g->last_node = NULL;

    return g;
}

FG_nodeList FG_NodeList(FG_node head, FG_nodeList tail)
{
    FG_nodeList n = (FG_nodeList) checked_malloc(sizeof *n);

    n->head = head;
    n->tail = tail;

    return n;
}

static FG_node FG_Node(FG_graph g, AS_instr instr)
{
    FG_node n = (FG_node)checked_malloc(sizeof *n);

    FG_nodeList p = FG_NodeList(n, NULL);

    assert(g);
    n->graph    = g;
    n->key      = g->nodecount++;

    if (g->last_node==NULL)
        g->nodes      = g->last_node       = p;
    else
        g->last_node  = g->last_node->tail = p;

    n->succs    = NULL;
    n->preds    = NULL;

    n->instr    = instr;

    switch (instr->mn)
    {
        case AS_ADD_Dn_Dn:                              //   1 add.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_ADD_Imm_Dn:                             //   2 add.x   #42, d2
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_ADD_Imm_sp:                             //   3 add.x  #42, sp
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_AND_Dn_Dn:                              //   4 and.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_AND_Imm_Dn:                             //   5 and.x  #23, d3
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_ASL_Dn_Dn:                              //   6 asl.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_ASR_Dn_Dn:                              //   8 asr.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_BEQ:                                    //  10 beq     label
        case AS_BNE:                                    //  11 bne     label
        case AS_BLT:                                    //  12 blt     label
        case AS_BGT:                                    //  13 bgt     label
        case AS_BLE:                                    //  14 ble     label
        case AS_BGE:                                    //  15 bge     label
        case AS_BCS:                                    //  16 bcs     label
        case AS_BHI:                                    //  17 bhi     label
        case AS_BLS:                                    //  18 bls     label
        case AS_BCC:                                    //  19 bcc     label
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_CMP_Dn_Dn:                              //  20 cmp.x   d0, d7
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_DIVS_Dn_Dn:                             //  21 divs.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_DIVS_Imm_Dn:                            //  22 divs.x  #23, d3
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_DIVU_Dn_Dn:                             //  23 divu.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_EOR_Dn_Dn:                              //  25 eor.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_EOR_Imm_Dn:                             //  26 eor.x  #23, d3
        case AS_EXT_Dn:                                 //  27 ext.x   d1
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_LSL_Dn_Dn:                              //  29 lsl.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_LSR_Dn_Dn:                              //  31 lsr.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_MOVE_AnDn_AnDn:                         //  33 move.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->dstInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        case AS_MOVE_Imm_RAn:                           //  35 move.x  #23, (a6)
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_dRegs();
            n->def       = Temp_TempSet(); 
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_MOVE_Imm_AnDn:                          //  36 move.x  #23, d0
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_AnDn_RAn:                          //  37 move.x  d1, (a6)
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_dRegs();
            n->def       = Temp_TempSet(); 
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_MOVE_RAn_AnDn:                          //  38 move.x  (a5), d1
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_dRegs();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        case AS_MOVE_AnDn_PDsp:                         //  41 move.x  d1, -(sp)
            assert(instr->src);
            assert(instr->dst==NULL);
            n->srcInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        case AS_MOVE_Imm_PDsp:                          //  42 move.x  #23, -(sp)
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_ILabel_AnDn:                       //  44 move.x  #label, d1
        case AS_MOVE_Label_AnDn:                        //  45 move.x  label, d6
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_AnDn_Label:                        //  46 move.x  d6, label
            assert(instr->src);
            assert(instr->dst==NULL);
            n->srcInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        case AS_MOVE_Ofp_AnDn:                          //  47 move.x  42(a5), d0
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_AnDn_Ofp:                          //  48 move.x  d0, 42(a5)
            assert(instr->src);
            assert(instr->dst==NULL);
            n->srcInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        case AS_MOVE_Imm_Ofp:                           //  49 move.x  #42, 42(a5)
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_Imm_Label:                         //  50 move.x  #42, label
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_MOVE_fp_AnDn:                           //  51 move.x  a5, d0
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = instr->w == AS_w_B ? F_aRegs() : Temp_TempSet();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet();
            break;

        case AS_MULS_Dn_Dn:                             //  52 muls.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_MULS_Imm_Dn:                            //  53 muls.x  #42, d2
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_MULU_Dn_Dn:                             //  54 mulu.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_NOT_Dn:                                 //  57 not.x   d0
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_NOP:                                    //  58 nop
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_OR_Dn_Dn:                               //  59 or.x  d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_OR_Imm_Dn:                              //  60 or.x  #42, d2
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_JMP:                                    //  61 jmp     label
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_JSR_Label:                              //  62 jsr     label
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            assert(instr->def);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();              // instr->def will be added, see below
            n->use       = Temp_TempSet();              // instr->use will be added, see below
            break;

        case AS_JSR_An:                                 //  63 jsr     (a2)
        case AS_JSR_RAn:                                //  64 jsr     -36(a6)
            assert(instr->src);
            assert(instr->dst==NULL);
            assert(instr->def);
            n->srcInterf = F_dRegs();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();              // instr->def will be added, see below
            n->use       = Temp_TempSet();              // instr->use will be added, see below
            break;

        case AS_RTS:                                    //  65 rts
            assert(instr->src==NULL);
            assert(instr->dst==NULL);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet();
            break;

        case AS_SNE_Dn:                                 //  66 sne.b   d1
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet();
            break;

        case AS_SUB_Dn_Dn:                              //  67 sub.x   d1, d2
            assert(instr->src);
            assert(instr->dst);
            n->srcInterf = F_aRegs();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_SUB_Imm_Dn:                             //  68 sub.x   #42, d2
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_SWAP_Dn:                                //  69 swap.x   d4
            assert(instr->src==NULL);
            assert(instr->dst);
            n->srcInterf = Temp_TempSet();
            n->dstInterf = F_aRegs();
            n->def       = Temp_TempSet(); Temp_tempSetAdd(n->def, instr->dst);
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->dst);
            break;

        case AS_TST_Dn:                                 //  70 tst.x   d0
            assert(instr->src);
            assert(instr->dst==NULL);
            n->srcInterf = F_aRegs();
            n->dstInterf = Temp_TempSet();
            n->def       = Temp_TempSet();
            n->use       = Temp_TempSet(); Temp_tempSetAdd(n->use, instr->src);
            break;

        default:
            EM_error(0, "*** internal error: unknown mn %d!", instr->mn);
            assert(0);
    }
    if (instr->def)
        n->def = Temp_tempSetUnion (n->def, instr->def);
    if (instr->use)
        n->use = Temp_tempSetUnion (n->use, instr->use);

    n->in       = Temp_TempSet();
    n->out      = Temp_TempSet();
#ifdef FG_DEPTH_FIRST_ORDER
    n->mark     = FALSE;
#endif

    return n;
}

static bool FG_inNodeList(FG_node a, FG_nodeList l)
{
    FG_nodeList p;
    for (p=l; p!=NULL; p=p->tail)
    {
        if (p->head==a)
            return TRUE;
    }
    return FALSE;
}

static bool FG_goesTo(FG_node from, FG_node n)
{
  return FG_inNodeList(n, from->succs);
}

void FG_addEdge(FG_node from, FG_node to)
{
    assert(from);
    assert(to);
    assert(from->graph == to->graph);

    if (FG_goesTo(from, to))
        return;
    to->preds   = FG_NodeList(from, to->preds);
    from->succs = FG_NodeList(to, from->succs);
}

static FG_node findLabeledNode(Temp_label lab, FG_nodeList nl, Temp_labelList ll)
{
    FG_node result = NULL;
    for (; nl && ll; nl = nl->tail, ll = ll->tail)
    {
        if (ll->head == lab)
        {
            result = nl->head;
            return result;
        }
    }
    return result;
}

FG_graph FG_AssemFlowGraph(AS_instrList il, F_frame f)
{
    FG_graph       g = FG_Graph();

    FG_nodeList    nl = NULL, jumpnl = NULL;
    Temp_labelList ll = NULL;
    FG_node        last_n = NULL, jump_n = NULL;
    AS_instr       last_inst = NULL, last_nonlbl_inst = NULL;

    // iterate and add instructions to graph
    for (; il; il = il->tail)
    {
        AS_instr inst = il->head;
        if (inst->mn != AS_LABEL)
        {
            FG_node n = FG_Node(g, inst);

            if (last_inst)
            {
				switch (last_inst->mn)
				{
        			case AS_LABEL:
						nl = FG_NodeList(n, nl);
						ll = Temp_LabelList(last_inst->label, ll);
						if (last_nonlbl_inst && (last_nonlbl_inst->mn != AS_JMP))
						{
							FG_addEdge(last_n, n);
						}
						break;
        			case AS_JMP:
						// no edge from last instruction to this one
						break;
        			default:
                        FG_addEdge(last_n, n);
				}
            }

			switch (inst->mn)
			{
				case AS_BEQ:
				case AS_BNE:
				case AS_BLT:
				case AS_BGT:
				case AS_BLE:
				case AS_BGE:
				case AS_JMP:
					jumpnl = FG_NodeList(n, jumpnl);
					break;

				default:
                    break;
			}

            last_n = n;
            last_nonlbl_inst = inst;
        }
        else
        {
            assert (!last_inst || (last_inst->mn != AS_LABEL)); // we cannot handle consequtive label instructions (codegen.c should have inserted NOPs here)
        }
        last_inst = inst;
    }

	// did we end on a label?
	if (last_inst && last_inst->mn == AS_LABEL)
	{
        // add a NOP node so the label points to something

        AS_instr nop = AS_Instr (AS_NOP, AS_w_NONE, NULL, NULL);
		FG_node n = FG_Node(g, nop);
		nl = FG_NodeList(n, nl);
		ll = Temp_LabelList(last_inst->label, ll);
        if (last_nonlbl_inst && (last_nonlbl_inst->mn != AS_JMP))
        {
            FG_addEdge(last_n, n);
        }
	}

    // handle jump instructions
    for (; jumpnl; jumpnl = jumpnl->tail)
    {
        FG_node n = jumpnl->head;
        jump_n = findLabeledNode(n->instr->label, nl, ll);
        if (jump_n)
        {
            FG_addEdge(n, jump_n);
        }
        else
        {
            EM_error(0, "failed to find node for label %s", Temp_labelstring(n->instr->label));
			assert(0);
        }
    }

    return g;
}

#define FG_COLUMN_1 20
#define FG_COLUMN_2 50
#define FG_COLUMN_3 80
#define FG_COLUMN_4 120
#define FG_COLUMN_5 160

void FG_show(FILE *out, FG_graph g, Temp_map tm)
{
    for (FG_nodeList p = g->nodes; p!=NULL; p=p->tail)
    {
        char buf[255];

        FG_node n = p->head;
        FG_nodeList q;
        assert(n);

        snprintf(buf, 255, " (%3d) -> ", n->key);

        int pos = strlen(buf);
        for (q=n->succs; q!=NULL; q=q->tail)
        {
            snprintf(&buf[pos], 255-pos, "%3d", q->head->key);
            pos += 3;
            if (q->tail)
            {
                buf[pos] = ',';
                pos++;
            }
        }

        while (pos<FG_COLUMN_1)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_1;

        snprintf(&buf[FG_COLUMN_1], 255-pos, " use: %s", Temp_tempSetSPrint(n->use, tm));

        pos = strlen(buf);
        while (pos<FG_COLUMN_2)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_2;

        snprintf(&buf[FG_COLUMN_2], 255-pos, " def: %s", Temp_tempSetSPrint(n->def, tm));

        pos = strlen(buf);
        while (pos<FG_COLUMN_3)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_3;

        snprintf(&buf[FG_COLUMN_3], 255-pos, " out: %s", Temp_tempSetSPrint(n->out, tm));

        pos = strlen(buf);
        while (pos<FG_COLUMN_4)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_4;

        snprintf(&buf[FG_COLUMN_4], 255-pos, " out: %s", Temp_tempSetSPrint(n->out, tm));

        pos = strlen(buf);
        while (pos<FG_COLUMN_5)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_5;

        AS_sprint(&buf[pos], n->instr, tm);
        pos = strlen(buf);
        buf[pos++] = '\n';
        buf[pos] = 0;
        fprintf(out, buf);
    }
}

#if 0
static void sprintLivemap(void* t, string buf)
{
    char buf2[255];
    G_node n = (G_node) t;
    Temp_tempList li, lo;
    AS_instr inst = (AS_instr) n->info;
    AS_sprint(buf2, inst, Temp_getNameMap());

    int l = strlen(buf2);
    while (l<30)
    {
        buf2[l] = ' ';
        l++;
    }
    buf2[l]=0;

    li = lookupLiveMap(g_in, n);
    lo = lookupLiveMap(g_out, n);

    sprintf(buf, "%s in: %s; out: %s", buf2, Temp_sprint_TempList(li), Temp_sprint_TempList(lo));
}
#endif

