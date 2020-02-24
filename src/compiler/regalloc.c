#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "color.h"
#include "flowgraph.h"
#include "liveness.h"
#include "regalloc.h"
#include "table.h"
#include "errormsg.h"

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
static void printTemp(void* t) {
   Temp_map m = Temp_name();
   printf("node: %s\n", Temp_look(m, (Temp_temp)t));
}

static void printInst(void *info) 
{
    AS_instr inst = (AS_instr)info;
    AS_print(stdout, inst, Temp_name());
}
#endif

static AS_instrList reverseInstrList(AS_instrList il) {
  AS_instrList rl = NULL;
  for (; il; il = il->tail) {
    rl = AS_InstrList(il->head, rl);
  }
  return rl;
}

static Temp_tempList inst_def(AS_instr inst) {
  switch (inst->kind) {
    case I_OPER:
      return inst->u.OPER.dst;
    case I_LABEL:
      return NULL;
    case I_MOVE:
      return inst->u.MOVE.dst;
  }
  return NULL;
}

static Temp_tempList inst_use(AS_instr inst) {
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

static G_node temp2Node(Temp_temp t, G_graph g) {
  if (t == NULL) return NULL;
  G_nodeList nodes = G_nodes(g);
  G_nodeList p;
  for(p=nodes; p!=NULL; p=p->tail)
    if (Live_gtemp(p->head)==t) return p->head;
  return NULL;
}

static Temp_temp node2Temp(G_node n) {
  if (n == NULL) return NULL;
  return Live_gtemp(n);
}

// static bool tempEqual(Temp_tempList ta, Temp_tempList tb) {
//   return Temp_equal(ta, tb);
// }

// static Temp_tempList tempMinus(Temp_tempList ta, Temp_tempList tb) {
//   return Temp_minus(ta, tb);
// }

static Temp_tempList tempUnion(Temp_tempList ta, Temp_tempList tb) {
  return Temp_union(ta, tb);
}

static Temp_tempList tempIntersect(Temp_tempList ta, Temp_tempList tb) {
  return Temp_intersect(ta, tb);
}

static bool tempIn(Temp_temp t, Temp_tempList tl) {
  return Temp_inList(t, tl);
}

static Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}

static bool instIn(AS_instr i, AS_instrList il) {
  return AS_instrInList(i, il);
}

static G_node getAlias(G_node n, G_table aliases, Temp_tempList coalescedNodes) {
  Temp_temp t = node2Temp(n);
  if (tempIn(t, coalescedNodes)) {
    G_node alias = (G_node)G_look(aliases, n);
    return getAlias(alias, aliases, coalescedNodes);
  } else {
    return n;
  }
}

static Temp_tempList aliased(Temp_tempList tl, G_graph ig,
                            G_table aliases, Temp_tempList cn) {
  Temp_tempList al = NULL;
  for (; tl; tl = tl->tail) {
    Temp_temp t = tl->head;
    G_node n = temp2Node(t, ig);
    getAlias(n, aliases, cn);
    t = node2Temp(n);
    al = L(t, al);
  }
  return tempUnion(al, NULL);
};

struct RA_result RA_regAlloc(F_frame f, AS_instrList il) 
{
    struct RA_result ret;
  
    G_graph flow;
    struct Live_graph live;
    Temp_map initial;
    struct COL_result col;
    AS_instrList rewriteList;
  
    int try = 0;
    while (++try < 7) 
    {
        flow = FG_AssemFlowGraph(il, f);
#ifdef ENABLE_DEBUG
        printf("try #%d flow graph:\n", try);
        printf("-----------------------\n");
        G_show(stdout, G_nodes(flow), printInst);
#endif
        live = Live_liveness(flow);
#ifdef ENABLE_DEBUG
        printf("try #%d liveness graph:\n", try);
        printf("-----------------------\n");
        G_show(stdout, G_nodes(live.graph), printTemp);
#endif
        initial = F_initialRegisters(f);
        col = COL_color(live.graph, initial, F_registers(),
                        live.worklistMoves, live.moveList, live.spillCost);
  
        if (col.spills == NULL) 
        {
            break;
        }
  
        Temp_tempList spilled = col.spills;
        rewriteList = NULL;
        
        // Assign locals in memory
        Temp_tempList tl;
        TAB_table spilledLocal = TAB_empty();
        for (tl = spilled; tl; tl = tl->tail) 
        {
            F_access local = F_allocLocal(f, Temp_ty(tl->head));
            TAB_enter(spilledLocal, tl->head, local);
        }
  
        // Rewrite instructions
        for (; il; il = il->tail) 
        {
            AS_instr inst = il->head;
            Temp_tempList useSpilled = tempIntersect(
                                        aliased(inst_use(inst), live.graph, col.alias, col.coalescedNodes),
                                        spilled);
            Temp_tempList defSpilled = tempIntersect(
                                        aliased(inst_def(inst), live.graph, col.alias, col.coalescedNodes),
                                        spilled);
            Temp_tempList tempSpilled = tempUnion(useSpilled, defSpilled);
  
            // Skip unspilled instructions
            if (tempSpilled == NULL) 
            {
                rewriteList = AS_InstrList(inst, rewriteList);
                continue;
            }
  
            for (tl = useSpilled; tl; tl = tl->tail) 
            {
                char buf[128];
                Temp_temp temp = tl->head;
                F_access local = (F_access)TAB_look(spilledLocal, temp);
                sprintf(buf, "move.l %d(`s0), `d0  /* spilled */\n", F_accessOffset(local));
                rewriteList = AS_InstrList(
                    AS_Oper(String(buf), L(temp, NULL), L(F_FP(), NULL), NULL), rewriteList);
            }
  
            rewriteList = AS_InstrList(inst, rewriteList);
  
            for (tl = defSpilled; tl; tl = tl->tail) 
            {
                char buf[128];
                Temp_temp temp = tl->head;
                F_access local = (F_access)TAB_look(spilledLocal, temp);
                sprintf(buf, "move.l `s0, %d(`s1)  /* spilled */\n", F_accessOffset(local));
                rewriteList = AS_InstrList(
                    AS_Oper(String(buf), NULL, L(temp, L(F_FP(), NULL)), NULL), rewriteList);
            }
        }
  
        il = reverseInstrList(rewriteList);
    }
  
    if (col.spills != NULL) 
    {
        EM_error(0, "fail to allocate registers");
    }
  
    if (col.coalescedMoves != NULL) 
    {
        rewriteList = NULL;
        for (; il; il = il->tail) 
        {
            AS_instr inst = il->head;
  
            // Remove coalesced moves
            if (instIn(inst, col.coalescedMoves)) 
            {
                char buf[1024];
                sprintf(buf, "# ");
                strcat(buf, inst->u.OPER.assem);
                inst->u.OPER.assem = String(buf);
                //continue;
            }
  
            rewriteList = AS_InstrList(inst, rewriteList);
        }
  
        il = reverseInstrList(rewriteList);
    }
  
    ret.coloring = col.coloring;
    ret.il = il;
  
    // Temp_tempList precolored = NULL;
    // Temp_tempList initial = NULL;
    // Temp_tempList simplifyWorklist = NULL;
    // Temp_tempList freezeWorklist = NULL;
    // Temp_tempList spillWorklist = NULL;
    // Temp_tempList spilledNodes = NULL;
    // Temp_tempList coalescedNodes = NULL;   // Coalesce
    // Temp_tempList coloredNodes = NULL;
    // Temp_tempList selectStack = NULL;
  
    // Temp_tempList worklistMoves = NULL; // Coalesce
  
    // do {
  
    // } while (simplifyWorklist != NULL || worklistMoves != NULL);
    
    return ret;
}
