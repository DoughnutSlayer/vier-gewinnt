#ifndef GAMEBOARD_H
#define GAMEBOARD_H

extern const int boardWidth;
extern const int boardHeigh;

struct gameboard
{
    int lanes[BOARD_WIDTH][BOARD_HEIGHT];
    int isWonBy;
    int nextPlayer;
};

struct gameboard *put(struct gameboard *board, int laneIndex);

#endif
