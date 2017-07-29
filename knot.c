#include <stdio.h>
#include <stdlib.h>
#include "gameboard.h"
#include "knot.h"

char* calculateHash(struct gameboard gameboard)
{
    char* hash = malloc((boardWidth * boardHeight) * sizeof(char));
    for (int lane = 0; lane < boardWidth; lane++)
    {
        for (int row = 0; row < boardHeight; row++)
        {
            char buffer[1];
            sprintf(buffer, "%d", gameboard.lanes[lane][row]);
            hash[(lane * boardWidth) + row] = buffer[0];
        }
    }
    return hash;
}
