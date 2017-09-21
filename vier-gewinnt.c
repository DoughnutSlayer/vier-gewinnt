#include <stdlib.h>
#include "mpi.h"
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"

int main(int argc, char *argv[])
{
    struct gameboard *rootBoard = malloc(sizeof(*rootBoard));
    initializeBoard(rootBoard);
    struct knot *root = createKnot(rootBoard);

    MPI_Init(&argc, &argv);

    buildParallelTree(root);

    MPI_Finalize();
    return 0;
}
