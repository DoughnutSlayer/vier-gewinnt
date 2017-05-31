#include <stdlib.h>

struct gameboard
{
    int lanes[4][4];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

struct gameboard *put(struct gameboard board, int laneIndex)
{
    int *updateLane = board.lanes[laneIndex];
    int laneSize = sizeof(updateLane)/sizeof(updateLane[0]);
    int rowIndex = 0;

    while(rowIndex < laneSize && updateLane[rowIndex] == 0)
    {
        rowIndex += 1;
    }

    if (rowIndex > 0)
    {
        struct gameboard *result = malloc(sizeof(*result));
        *result = board;
        result->lanes[laneIndex][rowIndex - 1] = result->nextPlayer;
        result->nextPlayer = 2 - (2 % result->nextPlayer);
        return result;
    }
    return NULL;
}
