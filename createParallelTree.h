#ifndef CREATEPARALLELTREE_H
#define CREATEPARALLELTREE_H

#include "gameboard.h"
#include "knot.h"

void buildParallelTree(struct knot *startKnot,
                       struct gameboard *startGameboard, struct knot **((*turnsPointer)[(BOARD_WIDTH * BOARD_HEIGHT) + 1]));

#endif
