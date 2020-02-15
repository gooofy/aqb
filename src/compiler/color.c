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
#include "color.h"
#include "liveness.h"
#include "table.h"

typedef struct ctx COL_ctx;

struct ctx {
  G_graph nodes;
  Temp_map precolored;
  Temp_tempList initial;
  Temp_tempList spillWorklist;
  Temp_tempList freezeWorklist;
  Temp_tempList simplifyWorklist;
  Temp_tempList spilledNodes;
  Temp_tempList coalescedNodes;
  Temp_tempList coloredNodes;
  Temp_tempList selectStack;

  AS_instrList coalescedMoves;
  AS_instrList constrainedMoves;
  AS_instrList frozenMoves;
  AS_instrList worklistMoves;
  AS_instrList activeMoves;

  Temp_map spillCost;
  Temp_map moveList;
  G_table alias;
  G_table degree;

  int K;
};

static COL_ctx c;

static Temp_tempList instDef(AS_instr inst) 
{
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

static Temp_tempList instUse(AS_instr inst) {
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

static Temp_tempList cloneRegs(Temp_tempList regs) {
  Temp_tempList tl = NULL;
  for (; regs; regs = regs->tail) {
    tl = Temp_TempList(regs->head, tl);
  }
  return tl;
}

static Temp_temp tempHead(Temp_tempList temps) {
  if (temps == NULL) return NULL;
  return temps->head;
}

static G_node temp2Node(Temp_temp t) {
  if (t == NULL) return NULL;
  G_nodeList nodes = G_nodes(c.nodes);
  G_nodeList p;
  for(p=nodes; p!=NULL; p=p->tail)
    if (Live_gtemp(p->head)==t) return p->head;
  return NULL;
}

static G_nodeList tempL2NodeL(Temp_tempList tl) {
  G_nodeList nl = NULL;
  for (; tl; tl = tl->tail) {
    nl = G_NodeList(temp2Node(tl->head), nl);
  }
  return G_reverseNodes(nl);
}

static Temp_temp node2Temp(G_node n) {
  if (n == NULL) return NULL;
  return Live_gtemp(n);
}

#if 0
static Temp_tempList nodeL2TempL(G_nodeList nl) {
  Temp_tempList tl = NULL;
  for (; nl; nl = nl->tail) {
    tl = Temp_TempList(node2Temp(nl->head), tl);
  }
  return Temp_reverseList(tl);
}
#endif

static string tempName(Temp_temp t) {
  return Temp_look(Temp_name(), t);
}

static Temp_temp str2Color(string color, Temp_map regcolors, Temp_tempList regs) {
  for (; regs; regs = regs->tail) {
    string s = Temp_look(regcolors, regs->head);
    if (s && (strcmp(s, color) == 0)) {
      return regs->head;
    }
  }
  //printf("register not found for given color: %s\n", color);
  return NULL;
}

static string color2Str(Temp_temp reg, Temp_map regcolors) {
  string color = Temp_look(regcolors, reg);
  if (color == NULL) {
    printf("color not found for given register: %s\n", tempName(reg));
  }
  return color;
}

static int tempCount(Temp_tempList t) {
  int cnt = 0;
  for (; t; t = t->tail) {
    ++cnt;
  }
  return cnt;
};

#if 0
static bool tempEqual(Temp_tempList ta, Temp_tempList tb) {
  return Temp_equal(ta, tb);
}
#endif

static Temp_tempList tempMinus(Temp_tempList ta, Temp_tempList tb) {
  return Temp_minus(ta, tb);
}

static Temp_tempList tempUnion(Temp_tempList ta, Temp_tempList tb) {
  return Temp_union(ta, tb);
}

#if 0
static Temp_tempList tempIntersect(Temp_tempList ta, Temp_tempList tb) {
  return Temp_intersect(ta, tb);
}
#endif

static bool tempIn(Temp_temp t, Temp_tempList tl) {
  return Temp_inList(t, tl);
}

static Temp_tempList L(Temp_temp h, Temp_tempList t) {
  return Temp_TempList(h, t);
}

static AS_instrList IL(AS_instr h, AS_instrList t) {
  return AS_InstrList(h, t);
}

static AS_instrList instMinus(AS_instrList ta, AS_instrList tb) {
  return AS_instrMinus(ta, tb);
}

static AS_instrList instUnion(AS_instrList ta, AS_instrList tb) {
  return AS_instrUnion(ta, tb);
}

static AS_instrList instIntersect(AS_instrList ta, AS_instrList tb) {
  return AS_instrIntersect(ta, tb);
}

static bool instIn(AS_instr i, AS_instrList il) {
  return AS_instrInList(i, il);
}


static Temp_tempList adjacent(Temp_temp t) {
  G_node n = temp2Node(t);
  G_nodeList adjn = G_adj(n);
  Temp_tempList adjs = NULL;
  for (; adjn; adjn = adjn->tail) {
    adjs = L(node2Temp(n), adjs);
  }

  adjs = tempMinus(adjs, tempUnion(c.selectStack, c.coalescedNodes));
  return adjs;
}

static void addEdge(G_node nu, G_node nv) {
  if (nu == nv) return;
  if (G_goesTo(nu, nv) || G_goesTo(nv, nu)) return;
  G_addEdge(nu, nv);

  Temp_temp u = node2Temp(nu);
  Temp_temp v = node2Temp(nv);

  if (Temp_look(c.precolored, u) == NULL) {
    long d = (long)G_look(c.degree, nu);
    d += 1;
    G_enter(c.degree, nu, (void*)d);
  }

  if (Temp_look(c.precolored, v) == NULL) {
    long d = (long)G_look(c.degree, nv);
    d += 1;
    G_enter(c.degree, nv, (void*)d);
  }
}

static AS_instrList nodeMoves(Temp_temp t) {
  AS_instrList ml = (AS_instrList)Temp_lookPtr(c.moveList, t);
  return instIntersect(ml, instUnion(c.activeMoves, c.worklistMoves));
}

static bool moveRelated(Temp_temp t) {
  return nodeMoves(t) != NULL;
}

static void makeWorkList() {
  Temp_tempList tl;
  for (tl = c.initial; tl; tl = tl->tail) {
    Temp_temp t = tl->head;
    G_node n = temp2Node(t);
    c.initial = tempMinus(c.initial, L(t, NULL));

    if (G_degree(n) >= c.K) {
      c.spillWorklist = tempUnion(c.spillWorklist, L(t, NULL));
    } else if (moveRelated(t)) {
      c.freezeWorklist = tempUnion(c.freezeWorklist, L(t, NULL));
    } else {
      c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(t, NULL));
    }
  }
}

#if 0
static void removeAdj(G_node n) {
  G_nodeList adjs = G_succ(n);
  for (; adjs; adjs = adjs->tail) {
    G_node m = adjs->head;
    G_rmEdge(n, m);
  }

  adjs = G_pred(n);
  for (; adjs; adjs = adjs->tail) {
    G_rmEdge(adjs->head, n);
  }
}
#endif

static void enableMoves(Temp_tempList tl) {
  for (; tl; tl = tl->tail) {
    AS_instrList il = nodeMoves(tl->head);
    for (; il; il = il->tail) {
      AS_instr m = il->head;
      if (instIn(m, c.activeMoves)) {
        c.activeMoves = instMinus(c.activeMoves, IL(m, NULL));
        c.worklistMoves = instUnion(c.worklistMoves, IL(m, NULL));
      }
    }
  }
}

static void decrementDegree(G_node n) {
  Temp_temp t = node2Temp(n);
  long d = (long)G_look(c.degree, n);
  d -= 1;
  G_enter(c.degree, n, (void*)d);

  if (d == c.K) {
    enableMoves(L(t, adjacent(t)));
    c.spillWorklist = tempMinus(c.spillWorklist, L(t, NULL));
    if (moveRelated(t)) {
      c.freezeWorklist = tempUnion(c.freezeWorklist, L(t, NULL));
    } else {
      c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(t, NULL));
    }
  }
}

static void addWorkList(Temp_temp t) {
  long degree = (long)G_look(c.degree, temp2Node(t));
  if (Temp_look(c.precolored, t) == NULL &&
      (!moveRelated(t)) &&
      (degree < c.K)) {
    c.freezeWorklist = tempMinus(c.freezeWorklist, L(t, NULL));
    c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(t, NULL));
  }
}

static bool OK(Temp_temp t, Temp_temp r) {
  G_node nt = temp2Node(t);
  G_node nr = temp2Node(r);
  long degree = (long)G_look(c.degree, nt);
  if (degree < c.K) {
    return TRUE;
  }
  if (Temp_look(c.precolored, t)) {
    return TRUE;
  }
  if (G_goesTo(nt, nr) || G_goesTo(nr, nt)) {
    return TRUE;
  }
  return FALSE;
}

static bool conservative(Temp_tempList tl) {
  G_nodeList nl = tempL2NodeL(tl);
  int k = 0;
  for (; nl; nl = nl->tail) {
    long degree = (long)G_look(c.degree, nl->head);
    if (degree >= c.K) {
      ++k;
    }
  }
  return (k < c.K);
}

static G_node getAlias(G_node n) {
  Temp_temp t = node2Temp(n);
  if (tempIn(t, c.coalescedNodes)) {
    G_node alias = (G_node)G_look(c.alias, n);
    return getAlias(alias);
  } else {
    return n;
  }
}

static void simplify() {
  if (c.simplifyWorklist == NULL) {
    return;
  }

  Temp_temp t = c.simplifyWorklist->head;
  G_node n = temp2Node(t);
  c.simplifyWorklist = c.simplifyWorklist->tail;

  c.selectStack = L(t, c.selectStack);  // push 

  G_nodeList adjs = G_adj(n);
  for (; adjs; adjs = adjs->tail) {
    G_node m = adjs->head;
    decrementDegree(m);
  }
}

static void combine(Temp_temp u, Temp_temp v) {
  G_node nu = temp2Node(u);
  G_node nv = temp2Node(v);
  if (tempIn(v, c.freezeWorklist)) {
    c.freezeWorklist = tempMinus(c.freezeWorklist, L(v, NULL));
  } else {
    c.spillWorklist = tempMinus(c.spillWorklist, L(v, NULL));
  }

  c.coalescedNodes = tempUnion(c.coalescedNodes, L(v, NULL));
  G_enter(c.alias, nv, (void*)nu);
  
  AS_instrList au = (AS_instrList)Temp_lookPtr(c.moveList, u);
  AS_instrList av = (AS_instrList)Temp_lookPtr(c.moveList, v);
  au = instUnion(au, av);
  Temp_enterPtr(c.moveList, u, (void*)au);

  enableMoves(L(v, NULL));

  //Temp_tempList tadjs = adjacent(v);
  //G_nodeList adjs = tempL2NodeL(tadjs);
  G_nodeList adjs = G_adj(nv);
  // FIXME remove? Temp_tempList tadjs = nodeL2TempL(adjs);
  for (; adjs; adjs = adjs->tail) {
    G_node nt = adjs->head;
    nt = getAlias(nt);
    addEdge(nt, nu);
    decrementDegree(nt);
  }
  // FIXME: remove? tadjs = NULL;

  long degree = (long)G_look(c.degree, nu);
  if (degree >= c.K && tempIn(u, c.freezeWorklist)) {
    c.freezeWorklist = tempMinus(c.freezeWorklist, L(u, NULL));
    c.spillWorklist = tempUnion(c.spillWorklist, L(u, NULL));
  }
}

static void coalesce() {
  if (c.worklistMoves == NULL) {
    return;
  }

  AS_instr m = c.worklistMoves->head;
  Temp_temp x = tempHead(instUse(m));
  Temp_temp y = tempHead(instDef(m));
  Temp_temp u, v;

  x = node2Temp(getAlias(temp2Node(x)));
  y = node2Temp(getAlias(temp2Node(y)));

  if (Temp_look(c.precolored, x) != NULL) {
    u = y; v = x;
  } else {
    u = x; v = y;
  }
  G_node nu = temp2Node(u);
  G_node nv = temp2Node(v);

  c.worklistMoves = instMinus(c.worklistMoves, IL(m, NULL));

  if (u == v) {
    c.coalescedMoves = instUnion(c.coalescedMoves, IL(m, NULL));
    addWorkList(u);
  } else if (Temp_look(c.precolored, v) || G_goesTo(nu, nv) || G_goesTo(nv, nu)) {
    c.constrainedMoves = instUnion(c.constrainedMoves, IL(m, NULL));
    addWorkList(u);
    addWorkList(v);
  } else {
    bool flag = FALSE;
    if (Temp_look(c.precolored, u)) {
      flag = TRUE;
      Temp_tempList adj = adjacent(v);
      for (; adj; adj = adj->tail) {
        if (!OK(adj->head, u)) {
          flag = FALSE;
          break;
        }
      }
    } else {
      Temp_tempList adju = adjacent(u);
      Temp_tempList adjv = adjacent(v);
      Temp_tempList adj = tempUnion(adju, adjv);
      flag = conservative(adj);
    }

    if (flag) {
      c.coalescedMoves = instUnion(c.coalescedMoves, IL(m, NULL));
      combine(u, v);
      addWorkList(u);
    } else {
      c.activeMoves = instUnion(c.activeMoves, IL(m, NULL));
    }
  }
}

static void freezeMoves(Temp_temp u) {
  AS_instrList il = nodeMoves(u);
  for (; il; il = il->tail) {
    AS_instr m = il->head;
    Temp_temp x = tempHead(instUse(m));
    Temp_temp y = tempHead(instDef(m));
    G_node nx = temp2Node(x);
    G_node ny = temp2Node(y);
    G_node nv;

    if (getAlias(nx) == getAlias(ny)) {
      nv = getAlias(nx);
    } else {
      nv = getAlias(ny);
    }
    Temp_temp v = node2Temp(nv);

    c.activeMoves = instMinus(c.activeMoves, IL(m, NULL));
    c.frozenMoves = instUnion(c.frozenMoves, IL(m, NULL));

    long degree = (long)G_look(c.degree, nv);
    if (nodeMoves(v) == NULL && degree < c.K) {
      c.freezeWorklist = tempMinus(c.freezeWorklist, L(v, NULL));
      c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(v, NULL));
    }
  }
}

static void freeze() {
  if (c.freezeWorklist == NULL) {
    return;
  }

  Temp_temp u = c.freezeWorklist->head;
  c.freezeWorklist = tempMinus(c.freezeWorklist, L(u, NULL));
  c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(u, NULL));
  freezeMoves(u);
}

static void selectSpill() {
  if (c.spillWorklist == NULL) {
    return;
  }
  Temp_tempList tl = c.spillWorklist;
  float minSpillPriority = 9999.0f;
  Temp_temp m = NULL;
  for (; tl; tl = tl->tail) {
    Temp_temp t = tl->head;
    long cost = (long)Temp_lookPtr(c.spillCost, t);
    long degree = (long)G_look(c.degree, temp2Node(t));
    degree = (degree > 0) ? degree : 1;
    float priority = ((float)cost) / degree;
    if (priority < minSpillPriority) {
      minSpillPriority = priority;
      m = t;
    }
  }
  c.spillWorklist = tempMinus(c.spillWorklist, L(m, NULL));
  c.simplifyWorklist = tempUnion(c.simplifyWorklist, L(m, NULL));
  freezeMoves(m);
}

static void colorMain() {
  makeWorkList();
  do {
    if (c.simplifyWorklist != NULL) {
      simplify();
    } else if (c.worklistMoves != NULL) {
      coalesce();
    } else if (c.freezeWorklist != NULL) {
      freeze();
    } else if (c.spillWorklist != NULL) {
      selectSpill();
    }
  } while (c.simplifyWorklist != NULL || c.worklistMoves != NULL ||
          c.freezeWorklist != NULL || c.spillWorklist != NULL);
}

struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs,
                            AS_instrList worklistMoves, Temp_map moveList, Temp_map spillCost) {
  //your code here.
  struct COL_result ret;

  c.precolored = initial;
  c.initial = NULL;
  c.simplifyWorklist = NULL;
  c.freezeWorklist = NULL;
  c.spillWorklist = NULL;
  c.spilledNodes = NULL;
  c.coalescedNodes = NULL;
  c.coloredNodes = NULL;
  c.selectStack = NULL;

  c.coalescedMoves = NULL;
  c.constrainedMoves = NULL;
  c.frozenMoves = NULL;
  c.worklistMoves = worklistMoves;
  c.activeMoves = NULL;

  c.spillCost = spillCost;
  c.moveList = moveList;
  c.degree = G_empty();
  c.alias = G_empty();
  c.nodes = ig;

  c.K = tempCount(regs);
  

  Temp_map precolored = initial;
  Temp_map colors = Temp_layerMap(Temp_empty(), initial);
  Temp_tempList coloredNodes = NULL;
  G_nodeList nodes = G_nodes(ig);

  G_nodeList nl;
  for (nl = nodes; nl; nl = nl->tail) {
    long degree = G_degree(nl->head);
    G_enter(c.degree, nl->head, (void*)degree);

    if (Temp_look(precolored, node2Temp(nl->head))) {
      G_enter(c.degree, nl->head, (void*)999);
      continue;
    }
    c.initial = L(node2Temp(nl->head), c.initial);
  }

  colorMain();

  // for (nl = nodes; nl; nl = nl->tail) {
  //   if (Temp_look(precolored, node2Temp(nl->head))) {
  //     continue;
  //   }
  //   c.selectStack = L(node2Temp(nl->head), c.selectStack);
  // }

  while (c.selectStack != NULL) {
    Temp_temp t = c.selectStack->head; // pop 
    G_node n = temp2Node(t);
    c.selectStack = c.selectStack->tail;

    Temp_tempList okColors = cloneRegs(regs);
    G_nodeList adjs = G_adj(n);
    G_node nw;
    // FIXME: remove? Temp_temp w;
    string color;

    int* it = (int*)t;
    if (*it == 119) {
      color = NULL;
    }
    
    for (; adjs; adjs = adjs->tail) {
      nw = adjs->head;
      // FIXME: remove? w = node2Temp(nw);
      G_node nw_alias = getAlias(nw);
      Temp_temp w_alias = node2Temp(nw_alias);
      if ((color = Temp_look(colors, w_alias)) != NULL) {
        Temp_temp colorTemp = str2Color(color, precolored, regs);
        if (colorTemp) {
          okColors = tempMinus(okColors, L(colorTemp, NULL));
        }
      }
    }

    if (okColors == NULL) {
      c.spilledNodes = L(t, c.spilledNodes);
    } else {
      coloredNodes = L(t, coloredNodes);
      Temp_enter(colors, t, color2Str(okColors->head, precolored));
    }
  }

  Temp_tempList tl;
  for (tl = c.coalescedNodes; tl; tl = tl->tail) {
    G_node alias = getAlias(temp2Node(tl->head));
    string color = Temp_look(colors, node2Temp(alias));
    Temp_enter(colors, tl->head, color);
  }

  ret.coloring = colors;
  
  ret.colored = NULL;
  for (; coloredNodes; coloredNodes = coloredNodes->tail) {
    ret.colored = L(coloredNodes->head, ret.colored);
  }

  ret.spills = NULL;
  for (; c.spilledNodes; c.spilledNodes = c.spilledNodes->tail) {
    printf("spilled: %s\n", tempName(c.spilledNodes->head));
    ret.spills = L(c.spilledNodes->head, ret.spills);
  }

  ret.coalescedMoves = c.coalescedMoves;
  ret.coalescedNodes = c.coalescedNodes;
  ret.alias = c.alias;

  return ret;
}

#if 0
static struct COL_result COL_color2(G_graph ig, Temp_map initial, Temp_tempList regs,
                            AS_instrList worklistMoves, Temp_map moveList, Temp_map spillCost) {
  //your code here.
  struct COL_result ret;

  Temp_map precolored = initial;
  Temp_map colors = Temp_layerMap(Temp_empty(), initial);
  G_nodeList spilledNodes = NULL, coloredNodes = NULL;
  G_nodeList nodes = G_nodes(ig);
  G_nodeList temps = NULL;

  G_nodeList nl;
  for (nl = nodes; nl; nl = nl->tail) {
    if (Temp_look(precolored, node2Temp(nl->head))) {
      continue;
    }
    temps = G_NodeList(nl->head, temps);
  }

  while (temps != NULL) {
    G_node n = temps->head;
    Temp_tempList okColors = cloneRegs(regs);
    G_nodeList adjs = G_adj(n);
    G_node adj;
    string color;
    
    for (; adjs; adjs = adjs->tail) {
      adj = adjs->head;
      if ((color = Temp_look(colors, node2Temp(adj))) != NULL) {
        Temp_temp colorTemp = str2Color(color, precolored, regs);
        if (colorTemp) {
          okColors = tempMinus(okColors,
                      Temp_TempList(colorTemp, NULL));
        }
      }
    }

    if (okColors == NULL) {
      spilledNodes = G_NodeList(n, spilledNodes);
    } else {
      coloredNodes = G_NodeList(n, coloredNodes);
      Temp_enter(colors, node2Temp(n), color2Str(okColors->head, precolored));
    }

    // Next
    temps = temps->tail;
  }

  ret.coloring = colors;
  
  ret.colored = NULL;
  for (; coloredNodes; coloredNodes = coloredNodes->tail) {
    ret.colored = Temp_TempList(node2Temp(coloredNodes->head), ret.colored);
  }

  ret.spills = NULL;
  for (; spilledNodes; spilledNodes = spilledNodes->tail) {
    printf("spilled: %s\n", tempName(node2Temp(spilledNodes->head)));
    ret.spills = Temp_TempList(node2Temp(spilledNodes->head), ret.spills);
  }

  ret.alias = G_empty();
  ret.coalescedMoves = NULL;
  ret.coalescedNodes = NULL;

  return ret;
}
#endif

