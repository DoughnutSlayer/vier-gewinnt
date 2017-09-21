#include <stdlib.h>
#include <stdio.h>
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
    printf("Winner: Player %d\n", board->isWonBy);
    printf("Next: Player %d\n", board->nextPlayer);
}

void printKnot(struct knot *knot, char *name)
{
    printGameboard(knot->gameboard, name);
    printf("Hash: %s\n", knot->gameboardHash);
    printf("Winpercentage: %f\n", knot->winPercentage);
}

void initializeBoard(struct gameboard *board)
{
    for (int i = 0; i < boardWidth; i++)
    {
        for (int j = 0; j < boardHeight; j++)
        {
            board->lanes[i][j] = 0;
        }
    }
    board->isWonBy = 0;
    board->nextPlayer = 1;
}

struct knot *createKnot(struct gameboard *board)
{
    struct knot *knot = malloc(sizeof(*knot));
    knot->gameboard = board;
    calculateHash(knot);
    knot->winPercentage = -1;
    return knot;
}
