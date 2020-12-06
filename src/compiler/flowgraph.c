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

    n->in       = Temp_TempSet();
    n->out      = Temp_TempSet();

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
    FG_graph      g = FG_Graph();
    FG_nodeList   nl = NULL, jumpnl = NULL;
    Temp_labelList ll = NULL;
    FG_node       last_n = NULL, jump_n = NULL;
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
		FG_node n = FG_Node(g, NULL);	// add a special NULL node so the label points to something

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

        while (pos<25)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = 25;

        snprintf(&buf[25], 255-pos, " in: %s", Temp_tempSetSPrint(n->in));

        pos = strlen(buf);
        while (pos<45)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = 45;

        snprintf(&buf[45], 255-pos, " out: %s", Temp_tempSetSPrint(n->out));

        pos = strlen(buf);
        while (pos<60)
        {
            buf[pos] = ' ';
            pos++;
        }
        pos = 60;

        if (n->instr)
        {
            AS_sprint(&buf[pos], n->instr, tm);
        }
        else
        {
            snprintf(&buf[pos], 255-pos, "NIL");
        }
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

