#include <stdio.h>
#include "util.h"
#include "symbol.h"
#include "temp.h"
#include "tree.h"
#include "absyn.h"
#include "assem.h"
#include "frame.h"
#include "graph.h"
#include "flowgraph.h"
#include "liveness.h"
#include "table.h"


Live_moveList Live_MoveList(G_node src, G_node dst, Live_moveList tail)
{
	Live_moveList lm = (Live_moveList) checked_malloc(sizeof(*lm));

    lm->src  = src;
	lm->dst  = dst;
	lm->tail = tail;

	return lm;
}

Temp_temp Live_gtemp(G_node n)
{
    return (Temp_temp)G_nodeInfo(n);
}

static bool tempEqual(Temp_tempList ta, Temp_tempList tb)
{
    return Temp_equal(ta, tb);
}

static Temp_tempList tempMinus(Temp_tempList ta, Temp_tempList tb)
{
    return Temp_minus(ta, tb);
}

static Temp_tempList tempUnion(Temp_tempList ta, Temp_tempList tb)
{
    return Temp_union(ta, tb);
}

#if 0
static Temp_tempList tempIntersect(Temp_tempList ta, Temp_tempList tb) {
  return Temp_intersect(ta, tb);
}

static Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}
#endif

static AS_instrList IL(AS_instr h, AS_instrList t) {
  return AS_InstrList(h, t);
}

#if 0
static AS_instrList instMinus(AS_instrList ta, AS_instrList tb) {
  return AS_instrMinus(ta, tb);
}
#endif

static AS_instrList instUnion(AS_instrList ta, AS_instrList tb) {
  return AS_instrUnion(ta, tb);
}

#if 0
static AS_instrList instIntersect(AS_instrList ta, AS_instrList tb) {
  return AS_instrIntersect(ta, tb);
}
#endif

static void enterLiveMap(G_table t, G_node flowNode, Temp_tempList temps) {
  G_enter(t, flowNode, temps);
}

static Temp_tempList lookupLiveMap(G_table t, G_node flownode) {
  return (Temp_tempList)G_look(t, flownode);
}

static void getLiveMap(G_graph flow, G_table in, G_table out) {
  G_nodeList fl, sl;
  G_node n, sn;
	G_table last_in = G_empty();
  G_table last_out = G_empty();
  Temp_tempList ci, co, li, lo;
  bool flag = TRUE;

	// Loop
	while (flag) {
    for (fl = G_nodes(flow); fl; fl = fl->tail) {
		  n = fl->head;
      li = lookupLiveMap(in, n);
      lo = lookupLiveMap(out, n);
      enterLiveMap(last_in, n, li);
      enterLiveMap(last_out, n, lo);

      ci = tempUnion(FG_use(n), tempMinus(lo, FG_def(n)));
      co = NULL;
      for (sl = G_succ(n); sl; sl = sl->tail) {
        sn = sl->head;
        co = tempUnion(co, lookupLiveMap(in, sn));
      }
      enterLiveMap(in, n, ci);
      enterLiveMap(out, n, co);
	  }

    flag = FALSE;
    for (fl = G_nodes(flow); fl; fl = fl->tail) {
      n = fl->head;
      li = lookupLiveMap(in, n);
      lo = lookupLiveMap(out, n);
      ci = lookupLiveMap(last_in, n);
      co = lookupLiveMap(last_out, n);

      if (!tempEqual(li, ci) || !tempEqual(lo, co)) {
        flag = TRUE;
        break;
      }
    }
	}
}

static G_node findOrCreateNode(Temp_temp t, G_graph g, TAB_table tab) {
  G_node ln = (G_node)TAB_look(tab, t);
  if (ln == NULL) {
    ln = G_Node(g, t);
    TAB_enter(tab, t, ln);
  }
  return ln;
}

static void solveLiveness(struct Live_graph *lg,
                          G_graph flow, G_table in, G_table out)
{
    G_graph g = G_Graph();
    TAB_table tab = TAB_empty();
    G_nodeList fl;
    G_node n, ndef, nedge, move_src=NULL;
    Temp_tempList tdef, tout, tuse, t, tedge;

    Temp_map moveList = Temp_empty();
    Temp_map spillCost = Temp_empty();
    AS_instr inst;
    AS_instrList worklistMoves = NULL;

    // Traverse node
    for (fl = G_nodes(flow); fl; fl = fl->tail)
    {
        n = fl->head;
        inst = FG_inst(n);
        tout = lookupLiveMap(out, n);
        tdef = FG_def(n);
        tuse = FG_use(n);

        Temp_tempList defuse = tempUnion(tuse, tdef);

        // Spill Cost
        for (t = defuse; t; t = t->tail)
        {
            Temp_temp ti = t->head;
            long spills = (long)Temp_lookPtr(spillCost, ti);
            ++spills;
            Temp_enterPtr(spillCost, ti, (void*)spills);
        }

        // Move instruction?
        if (FG_isMove(n) && tdef)
        {
            for (; defuse; defuse = defuse->tail)
            {
                Temp_temp t = defuse->head;
                findOrCreateNode(t, g, tab);
                AS_instrList ml = (AS_instrList)Temp_lookPtr(moveList, t);
                ml = instUnion(ml, IL(inst, NULL));
                Temp_enterPtr(moveList, t, (void*)ml);
            }

            worklistMoves = instUnion(worklistMoves, IL(inst, NULL));
        }

        // Traverse defined vars
        for (t = tout; t; t = t->tail)
        {
            ndef = findOrCreateNode(t->head, g, tab);

            // Add edges between output vars and defined var
            for (tedge = tout; tedge; tedge = tedge->tail)
            {
                nedge = findOrCreateNode(tedge->head, g, tab);

                // Skip if edge is added
                if (ndef == nedge || G_goesTo(ndef, nedge) || G_goesTo(nedge, ndef))
                {
                    continue;
                }

                // Skip src for move instruction
                if (FG_isMove(n) && nedge == move_src)
                    continue;

                G_addEdge(ndef, nedge);
            }
        }
    }

    lg->graph = g;
    lg->worklistMoves = worklistMoves;
    lg->moveList = moveList;
    lg->spillCost = spillCost;
}

#if 0
static void solveLiveness3(struct Live_graph *lg,
                          G_graph flow, G_table in, G_table out) {
  G_graph g = G_Graph();
  TAB_table tab = TAB_empty();
  Temp_map moveList = Temp_empty();
  Temp_map spillCost = Temp_empty();
  AS_instrList worklistMoves = NULL;
  G_nodeList fl;
  G_node n, ndef, nedge;
  Temp_tempList tdef, tuse, tl, tedge, live = NULL;
  AS_instr inst;
  bool blockStart = TRUE;

  // Traverse node
  fl = G_reverseNodes(G_nodes(flow));
  //if (fl) live = lookupLiveMap(out, fl->head);
  for (; fl; fl = fl->tail) {
    if (blockStart) {
      live = lookupLiveMap(out, fl->head);
      blockStart = FALSE;
    }

    n = fl->head;
    inst = FG_inst(n);
    tuse = FG_use(n);
    tdef = FG_def(n);

    if (inst->kind == I_LABEL) {
       blockStart = TRUE;
       continue;
    }

    Temp_tempList defuse = tempUnion(tuse, tdef);

    // Spill Cost
    for (tl = defuse; tl; tl = tl->tail) {
      Temp_temp ti = tl->head;
      long spills = (long)Temp_lookPtr(spillCost, ti);
      ++spills;
      Temp_enterPtr(spillCost, ti, (void*)spills);
    }

    // Move instruction?
    if (FG_isMove(n)) {
      live = tempMinus(live, tuse);

      for (; defuse; defuse = defuse->tail) {
        Temp_temp t = defuse->head;
        AS_instrList ml = (AS_instrList)Temp_lookPtr(moveList, t);
        ml = instUnion(ml, IL(inst, NULL));
        Temp_enterPtr(moveList, t, (void*)ml);
      }

      worklistMoves = instUnion(worklistMoves, IL(inst, NULL));
    }

    live = tempUnion(live, tdef);

    // Traverse defined vars
    for (tl = tdef; tl; tl = tl->tail) {
      ndef = findOrCreateNode(tl->head, g, tab);

      // Add edges between output vars and defined var
      for (tedge = live; tedge; tedge = tedge->tail) {
        nedge = findOrCreateNode(tedge->head, g, tab);

        // Skip if edge is added
        if (ndef == nedge || G_goesTo(ndef, nedge) || G_goesTo(nedge, ndef)) {
          continue;
        }

        G_addEdge(ndef, nedge);
      }
    }

    live = tempUnion(tuse, tempMinus(live, tdef));
  }

  lg->graph = g;
  lg->worklistMoves = worklistMoves;
  lg->moveList = moveList;
  lg->spillCost = spillCost;
}

static void solveLiveness2(struct Live_graph *lg,
                          G_graph flow, G_table in, G_table out)
{
  G_graph g = G_Graph();
  TAB_table tab = TAB_empty();
  Live_moveList ml = NULL;
  G_nodeList fl;
  G_node n, ndef, nedge, move_src, move_dst;
  Temp_tempList tout, t, tedge;

  // Traverse node
  for (fl = G_nodes(flow); fl; fl = fl->tail) {
    n = fl->head;
    tout = lookupLiveMap(out, n);
    // FIXME: remove ? tdef = FG_def(n);

    // Move instruction?
    if (FG_isMove(n)) {
      move_src = findOrCreateNode(FG_use(n)->head, g, tab);
      move_dst = findOrCreateNode(FG_def(n)->head, g, tab);
      ml = Live_MoveList(move_src, move_dst, ml);
    }

    // Traverse defined vars
    for (t = tout; t; t = t->tail) {
      ndef = findOrCreateNode(t->head, g, tab);

      // Add edges between output vars and defined var
      for (tedge = tout; tedge; tedge = tedge->tail) {
        nedge = findOrCreateNode(tedge->head, g, tab);

        // Skip if edge is added
        if (ndef == nedge || G_goesTo(ndef, nedge) || G_goesTo(nedge, ndef)) {
          continue;
        }

        // Skip src for move instruction
        if (FG_isMove(n) && nedge == move_src) {
          continue;
        }

        G_addEdge(ndef, nedge);
      }
    }
  }

  lg->graph = g;
  lg->moves = ml;
}
#endif

struct Live_graph Live_liveness(G_graph flow)
{
    // Construct liveness graph
    G_table in = G_empty(), out = G_empty();
    getLiveMap(flow, in, out);

    // Construct interference graph
    struct Live_graph lg;
    solveLiveness(&lg, flow, in, out);

	return lg;
}


