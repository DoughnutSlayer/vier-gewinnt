#include <stdlib.h>
#include <stdio.h>
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"
#include "testHelp.h"

int main(int argc, char *argv[])
{
    struct gameboard *rootBoard = malloc(sizeof(*rootBoard));
    initializeBoard(rootBoard);
    struct knot *root = createKnot(rootBoard);
    buildParallelTree(argc, argv, root);
    return 0;
}
