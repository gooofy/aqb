#include <stdio.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "errormsg.h"
#include "table.h"

Temp_tempList FG_def(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    return inst ? inst->dst : NULL;
}

Temp_tempList FG_use(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    return inst ? inst->src : NULL;
}

bool FG_isMove(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    if (!inst)
        return FALSE;
    return inst->mn == AS_MOVE_AnDn_AnDn;
}

Temp_tempLList FG_interferingRegsDef(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    if (!inst)
        return NULL;
    return inst->dstInterf;
}

Temp_tempLList FG_interferingRegsUse(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    if (!inst)
        return NULL;
    return inst->srcInterf;
}

AS_instr FG_inst(G_node n)
{
    return (AS_instr)G_nodeInfo(n);
}

static G_node findLabeledNode(Temp_label lab, G_nodeList nl, Temp_labelList ll)
{
    G_node result = NULL;
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

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f)
{
    G_graph        g = G_Graph();
    G_nodeList     nl = NULL, jumpnl = NULL;
    Temp_labelList ll = NULL;
    G_node         last_n = NULL, jump_n = NULL;
    AS_instr       inst = NULL, last_inst = NULL, last_nonlbl_inst = NULL;

    // iterate and add instructions to graph
    for (; il; il = il->tail)
    {
        inst = il->head;
        if (inst->mn != AS_LABEL)
        {
            G_node n = G_Node(g, (void*)inst);

            if (last_inst)
            {
				switch (last_inst->mn)
				{
        			case AS_LABEL:
						nl = G_NodeList(n, nl);
						ll = Temp_LabelList(last_inst->label, ll);
						if (last_nonlbl_inst && (last_nonlbl_inst->mn != AS_JMP))
						{
							G_addEdge(last_n, n);
						}
						break;
        			case AS_JMP:
						// no edge from last instruction to this one
						break;
        			default:
                        G_addEdge(last_n, n);
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
					jumpnl = G_NodeList(n, jumpnl);
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
		G_node n = G_Node(g, NULL);	// add a special NULL node so the label points to something

		nl = G_NodeList(n, nl);
		ll = Temp_LabelList(last_inst->label, ll);
        if (last_nonlbl_inst && (last_nonlbl_inst->mn != AS_JMP))
        {
            G_addEdge(last_n, n);
        }
	}

    // handle jump instructions
    for (; jumpnl; jumpnl = jumpnl->tail)
    {
        G_node n = jumpnl->head;
        inst = (AS_instr) G_nodeInfo(n);
        jump_n = findLabeledNode(inst->label, nl, ll);
        if (jump_n)
        {
            G_addEdge(n, jump_n);
        }
        else
        {
            EM_error(0, "failed to find node for label %s", Temp_labelstring(inst->label));
			assert(0);
        }
    }

    return g;
}
