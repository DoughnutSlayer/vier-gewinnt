#include <stdio.h>
#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth;
extern const int boardHeight;

void calculateHash(struct knot *knot)
{
    for (int lane = 0; lane < boardWidth; lane++)
    {
        for (int row = 0; row < boardHeight; row++)
        {
            char buffer[1];
            sprintf(buffer, "%d", knot->gameboard->lanes[lane][row]);
            int index = (lane * boardWidth) + row;
            knot->gameboardHash[index] = buffer[0];
        }
    }
}
