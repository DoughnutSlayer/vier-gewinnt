#include <stdio.h>
#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;
extern const int boardHeight;

void printGameboard(struct gameboard *board, char *name)
{
    printf("%s:\n", name);
    for (int rowIndex = 0; rowIndex < boardHeight; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < boardWidth; columnIndex++)
        {
            printf("%d\t", board->lanes[columnIndex][rowIndex]);
        }
        printf("\n");
    }
    printf("Predecessor Index: %d\n", board->predecessorIndex);
    printf("Hash: %s\n", board->hash);
    printf("Winner: Player %d\n", board->isWonBy);
    printf("Next: Player %d\n", board->nextPlayer);
}

void printKnot(struct knot *knot, char *name)
{
    printf("%s:\n", name);
    printf("Successor Indices:");
    for (int i = 0; i < boardWidth; i++)
    {
        printf("\t[ %d = %d ]", i, knot->successorIndices[i]);
    }
    printf("\nWinpercentage: %f\n", knot->winPercentage);
}
