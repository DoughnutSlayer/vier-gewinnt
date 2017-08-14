#include <stdio.h>
#include <stdlib.h>
#include "calculateSequentialWinPercentage.h"
#include "createSequentialTree.h"
#include "gameboard.h"
#include "knot.h"
#include "testHelp.h"

int main(int argc, char const *argv[])
{
    struct gameboard *board = malloc(sizeof(*board));
    initializeBoard(board);
    struct knot *knot = createKnot(board);
    buildTree(knot);
    calculateWinPercentage(knot);
    return 0;
}
