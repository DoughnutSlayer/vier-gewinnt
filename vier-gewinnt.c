#include <stdlib.h>
#include "mpi.h"
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"

struct knot playerKnot;

int main(int argc, char *argv[])
{
    struct gameboard emptyBoard = {0};
    emptyBoard.nextPlayer = 1;
    playerKnot.gameboard = &emptyBoard;

    MPI_Init(&argc, &argv);

    buildParallelTree(root);

    MPI_Finalize();
    return 0;
}
