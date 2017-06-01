#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#ifndef BOARD_HEIGHT
#define BOARD_HEIGHT 4
#endif

#ifndef BOARD_WIDTH
#define BOARD_WIDTH 4
#endif

const int boardWidth = BOARD_WIDTH;
const int boardHeight = BOARD_HEIGHT;

struct gameboard
{
    int lanes[BOARD_WIDTH][BOARD_HEIGHT];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

struct gameboard *put(struct gameboard *board, int laneIndex);

#endif
