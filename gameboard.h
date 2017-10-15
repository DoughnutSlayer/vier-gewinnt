#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#ifndef BOARD_HEIGHT
#define BOARD_HEIGHT 4
#endif

#ifndef BOARD_WIDTH
#define BOARD_WIDTH 4
#endif

extern const int boardWidth;
extern const int boardHeight;

struct gameboard
{
    int predecessorIndex;
    int lanes[BOARD_WIDTH][BOARD_HEIGHT];
    char hash[(BOARD_WIDTH * BOARD_HEIGHT) + 1];
    int isWonBy;
    int nextPlayer;
};

struct gameboard *put(struct gameboard *board, int laneIndex);
void calculateHash(struct gameboard *board);

#endif
