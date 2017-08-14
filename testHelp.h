#ifndef TESTHELP_H
#define TESTHELP_H

#include "gameboard.h"

void printGameboard(struct gameboard *board, char *name);

void initializeBoard(struct gameboard *board);

struct knot *createKnot(struct gameboard *board);

#endif
