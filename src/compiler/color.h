#ifndef HAVE_COLOR_H
#define HAVE_COLOR_H


/*
 * color.h - Data structures and function prototypes for coloring algorithm
 *             to determine register allocation.
 */

struct COL_result {Temp_map coloring; Temp_tempList colored; Temp_tempList spills;
                    AS_instrList coalescedMoves; Temp_tempList coalescedNodes; G_table alias;};
struct COL_result COL_color(G_graph ig, Temp_map initial, Temp_tempList regs,
                            AS_instrList worklistMoves, Temp_map moveList, Temp_map spillCost);

#endif

