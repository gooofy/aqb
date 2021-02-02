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

    n->def      = NULL;
    n->use      = NULL;

    if (AS_instrInfoA[instr->mn].hasSrc)
    {
        n->use = Temp_TempSet (instr->src, n->use);
        if (AS_instrInfoA[instr->mn].srcDnOnly)
        {
            n->srcInterf = AS_aRegs();
        }
        else
        {
            if (AS_instrInfoA[instr->mn].srcAnOnly)
                n->srcInterf = AS_dRegs();
            else
                n->srcInterf = instr->w == Temp_w_B ? AS_aRegs() : NULL;
        }
    }
    else
    {
        n->srcInterf = NULL;
    }

    if (AS_instrInfoA[instr->mn].hasDst)
    {
        if (!AS_instrInfoA[instr->mn].dstIsOnlySrc)
            n->def = Temp_TempSet (instr->dst, n->def);
        if (AS_instrInfoA[instr->mn].dstIsAlsoSrc)
            n->use = Temp_TempSet (instr->dst, n->use);

        if (AS_instrInfoA[instr->mn].dstDnOnly)
        {
            n->dstInterf = AS_aRegs();
        }
        else
        {
            if (AS_instrInfoA[instr->mn].dstAnOnly)
                n->dstInterf = AS_dRegs();
            else
                n->dstInterf = instr->w == Temp_w_B ? AS_aRegs() : NULL;
        }
    }
    else
    {
        n->dstInterf = NULL;
    }

    if (instr->def)
        n->def = Temp_tempSetUnion (n->def, instr->def);
    if (instr->use)
        n->use = Temp_tempSetUnion (n->use, instr->use);

    n->in       = NULL;
    n->out      = NULL;
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

FG_graph FG_AssemFlowGraph(AS_instrList il)
{
    FG_graph       g = FG_Graph();

    FG_nodeList    nl = NULL, jumpnl = NULL;
    Temp_labelList ll = NULL;
    FG_node        last_n = NULL, jump_n = NULL;
    AS_instr       last_inst = NULL, last_nonlbl_inst = NULL;

    // iterate and add instructions to graph
    for (AS_instrListNode an = il->first; an; an=an->next)
    {
        AS_instr inst = an->instr;
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

        AS_instr nop = AS_Instr (last_inst->pos, AS_NOP, Temp_w_NONE, NULL, NULL);
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

void FG_free (FG_graph g)
{
    FG_nodeList nl_tail = NULL;
    for (FG_nodeList nl=g->nodes; nl; nl=nl_tail)
    {
        U_memfree (nl->head, sizeof (*nl->head));
        nl_tail = nl->tail;
        U_memfree (nl, sizeof (*nl));
    }
}

#define FG_COLUMN_1 20
#define FG_COLUMN_2 50
#define FG_COLUMN_3 80
#define FG_COLUMN_4 120
#define FG_COLUMN_5 160

void FG_show(FILE *out, FG_graph g)
{
    for (FG_nodeList p = g->nodes; p!=NULL; p=p->tail)
    {
        char buf[255];

        FG_node n = p->head;
        FG_nodeList q;
        assert(n);

        snprintf(buf, 255, " (%4d) -> ", n->key);

        int pos = strlen(buf);
        for (q=n->succs; q!=NULL; q=q->tail)
        {
            snprintf(&buf[pos], 255-pos, "%4d", q->head->key);
            pos += 4;
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

        snprintf(&buf[FG_COLUMN_1], 255-pos, " use: %s", Temp_tempSetSPrint(n->use));

        pos = strlen(buf);
        while (pos<FG_COLUMN_2)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_2;

        snprintf(&buf[FG_COLUMN_2], 255-pos, " def: %s", Temp_tempSetSPrint(n->def));

        pos = strlen(buf);
        while (pos<FG_COLUMN_3)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_3;

        snprintf(&buf[FG_COLUMN_3], 255-pos, " in: %s", Temp_tempSetSPrint(n->in));

        pos = strlen(buf);
        while (pos<FG_COLUMN_4)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_4;

        snprintf(&buf[FG_COLUMN_4], 255-pos, " out: %s", Temp_tempSetSPrint(n->out));

        pos = strlen(buf);
        while (pos<FG_COLUMN_5)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = FG_COLUMN_5;

        AS_sprint(&buf[pos], n->instr, AS_dialect_gas);
        pos = strlen(buf);
        buf[pos++] = '\n';
        buf[pos] = 0;
        fprintf(out, buf);
    }
}

