#ifndef CREATEPARALLELTREE_H
#define CREATEPARALLELTREE_H

#include "gameboard.h"
#include "knot.h"

extern int turnSizes[(BOARD_WIDTH * BOARD_HEIGHT) + 1];
extern int turnDisplacements[(BOARD_WIDTH * BOARD_HEIGHT) + 1];

void buildParallelTree(struct knot *startKnot,
                       struct gameboard *startGameboard);

#endif
