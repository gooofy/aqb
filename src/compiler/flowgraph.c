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
    switch (inst->kind)
    {
        case I_OPER:
            return inst->u.OPER.dst;
       case I_LABEL:
            return NULL;
        case I_MOVE:
            return inst->u.MOVE.dst;
    }
    return NULL;
}

Temp_tempList FG_use(G_node n) {
  AS_instr inst = (AS_instr)G_nodeInfo(n);
  switch (inst->kind) {
    case I_OPER:
      return inst->u.OPER.src;
    case I_LABEL:
      return NULL;
    case I_MOVE:
      return inst->u.MOVE.src;
  }
  return NULL;
}

bool FG_isMove(G_node n)
{
    AS_instr inst = (AS_instr)G_nodeInfo(n);
    return (inst->kind == I_MOVE);
}

AS_instr FG_inst(G_node n) {
  return (AS_instr)G_nodeInfo(n);
}

static G_node findLabeledNode(Temp_label lab, G_nodeList nl, Temp_labelList ll) {
  G_node result = NULL;
  for (; nl && ll; nl = nl->tail, ll = ll->tail) {
    if (ll->head == lab) {
      result = nl->head;
      return result;
    }
  }
  return result;
}

G_graph FG_AssemFlowGraph(AS_instrList il, F_frame f)
{
    G_graph g = G_Graph();
    G_nodeList nl = NULL, jumpnl = NULL;
    Temp_labelList ll = NULL, jl = NULL;
    G_node n = NULL, last_n = NULL, jump_n = NULL;
    AS_instr inst = NULL, last_inst = NULL, last_nonlbl_inst = NULL;

    // Iterate and add instructions to graph
    for (; il; il = il->tail)
    {
        inst = il->head;
        if (inst->kind != I_LABEL)
        {
            n = G_Node(g, (void*)inst);

            if (last_inst)
            {
                if (last_inst->kind == I_LABEL)
                {
                    nl = G_NodeList(n, nl);
                    ll = Temp_LabelList(last_inst->u.LABEL.label, ll);
                    if (last_nonlbl_inst)
                    {
                        G_addEdge(last_n, n);
                    }
                }
                else
                {
                    if (last_inst->kind == I_OPER && last_inst->u.OPER.target != NULL)
                    {
                        // add edge for conditional jumps
                        if (strstr(last_inst->u.OPER.assem, "jmp") != last_inst->u.OPER.assem)
                        {
                            G_addEdge(last_n, n);
                        }
                    }
                    else
                    {
                        G_addEdge(last_n, n);
                    }
                }
            }

            if (inst->kind == I_OPER && inst->u.OPER.target != NULL)
            {
                jumpnl = G_NodeList(n, jumpnl);
            }

            last_n = n;
            last_nonlbl_inst = inst;
        }
        last_inst = inst;
    }

    // Handle jump instructions
    for (; jumpnl; jumpnl = jumpnl->tail)
    {
        n = jumpnl->head;
        inst = (AS_instr) G_nodeInfo(n);
        jump_n = findLabeledNode(inst->u.OPER.target, nl, ll);
        if (jump_n)
        {
            G_addEdge(n, jump_n);
        }
        else
        {
            EM_error(0, "fail to find node for label %s", Temp_labelstring(jl->head));
        }
    }

    return g;
}
