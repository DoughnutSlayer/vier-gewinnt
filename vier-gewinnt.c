#include <stdlib.h>

struct gameboard
{
    int lanes[4][4];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

void updateGameFinishedStatus(struct gameboard *board)
{
    //TODO: Die Felder isWonBy und isFinished basierend auf den lanes des gegebenen gameboards aktualisieren.
}

struct gameboard *put(struct gameboard *board, int laneIndex)
{
    if (board->isFinished)
    {
        return NULL;
    }
    
    int *updateLane = board->lanes[laneIndex];
    int laneSize = sizeof(board->lanes[laneIndex])/sizeof(board->lanes[laneIndex][0]);
    int rowIndex = 0;

    while(rowIndex < laneSize && updateLane[rowIndex] == 0)
    {
        rowIndex += 1;
    }

    if (rowIndex > 0)
    {
        struct gameboard *result = malloc(sizeof(*result));
        *result = *board;
        result->lanes[laneIndex][rowIndex - 1] = result->nextPlayer;
        result->nextPlayer = 1 + (result->nextPlayer % 2);
        updateGameFinishedStatus(result);
        return result;
    }
    return NULL;
}
