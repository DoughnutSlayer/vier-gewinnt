#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"
#include "testHelp.h"

int main(int argc, char const *argv[])
{
    struct gameboard *rootBoard = malloc(sizeof(*rootBoard));
    struct knot *root = createKnot(rootBoard);
    buildParallelTree(root);
    return 0;
}
