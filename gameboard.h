#ifndef GAMEBOARD_H
#define GAMEBOARD_H

struct gameboard
{
    int lanes[4][4];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

struct gameboard *put(struct gameboard *board, int laneIndex);

#endif
