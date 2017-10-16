#include "gameboard.h"
#include <stdio.h>
#include <stdlib.h>

const int boardWidth = BOARD_WIDTH;
const int boardHeight = BOARD_HEIGHT;

void calculateHash(struct gameboard *board)
{
    for (int lane = 0; lane < boardWidth; lane++)
    {
        for (int row = 0; row < boardHeight; row++)
        {
            char buffer[1];
            sprintf(buffer, "%d", board->lanes[lane][row]);
            int index = (lane * boardHeight) + row;
            board->hash[index] = buffer[0];
        }
    }
    board->hash[boardWidth * boardHeight] = '\0';
}

int updateSearchStatus(int *currentPlayer, int *foundPieces, int currentPiece,
                       struct gameboard *board)
{
    if (!currentPiece)
    {
        *currentPlayer = 0;
        *foundPieces = 0;
    }
    else if (*currentPlayer != currentPiece)
    {
        *currentPlayer = currentPiece;
        *foundPieces = 1;
    }
    else
    {
        ++*foundPieces;
        if (*foundPieces >= 4)
        {
            board->isWonBy = *currentPlayer;
            return 1;
        }
    }
    return 0;
}

int checkRowsForMatch(int columnCount, int rowCount, struct gameboard *board)
{
    if (columnCount < 4)
    {
        return 0;
    }
    int currentPlayer;
    int foundPieces;
    for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
        currentPlayer = 0;
        foundPieces = 0;
        for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
        {
            if (updateSearchStatus(&currentPlayer, &foundPieces,
                                   board->lanes[columnIndex][rowIndex], board))
            {
                return 1;
            }
        }
    }
    return 0;
}

int checkColumnsForMatch(int columnCount, int rowCount, struct gameboard *board)
{
    if (rowCount < 4)
    {
        return 0;
    }
    int currentPlayer;
    int foundPieces;
    for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
    {
        currentPlayer = 0;
        foundPieces = 0;
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            if (updateSearchStatus(&currentPlayer, &foundPieces,
                                   board->lanes[columnIndex][rowIndex], board))
            {
                return 1;
            }
        }
    }
    return 0;
}

int checkForwardDiagonalsForMatch(int columnCount, int rowCount,
                                  struct gameboard *board)
{
    if (columnCount < 4 || rowCount < 4)
    {
        return 0;
    }
    int rowAnchor = 3;
    int columnAnchor = 0;
    int rowIndex;
    int columnIndex;
    int currentPlayer;
    int foundPieces;
    while (columnAnchor <= columnCount - 4)
    {
        rowIndex = rowAnchor;
        columnIndex = columnAnchor;
        currentPlayer = 0;
        foundPieces = 0;
        while (rowIndex >= 0 && columnIndex < columnCount)
        {
            if (updateSearchStatus(&currentPlayer, &foundPieces,
                                   board->lanes[columnIndex][rowIndex], board))
            {
                return 1;
            }
            rowIndex--;
            columnIndex++;
        }
        if (rowAnchor < rowCount - 1)
        {
            rowAnchor++;
        }
        else
        {
            columnAnchor++;
        }
    }
    return 0;
}

int checkBackwardDiagonalsForMatch(int columnCount, int rowCount,
                                   struct gameboard *board)
{
    if (columnCount < 4 || rowCount < 4)
    {
        return 0;
    }
    int rowAnchor = 3;
    int columnAnchor = columnCount - 1;
    int rowIndex;
    int columnIndex;
    int currentPlayer;
    int foundPieces;
    while (columnAnchor >= 3)
    {
        rowIndex = rowAnchor;
        columnIndex = columnAnchor;
        currentPlayer = 0;
        foundPieces = 0;
        while (rowIndex >= 0 && columnIndex >= 0)
        {
            if (updateSearchStatus(&currentPlayer, &foundPieces,
                                   board->lanes[columnIndex][rowIndex], board))
            {
                return 1;
            }
            rowIndex--;
            columnIndex--;
        }
        if (rowAnchor < rowCount - 1)
        {
            rowAnchor++;
        }
        else
        {
            columnAnchor--;
        }
    }
    return 0;
}

int boardIsFull(struct gameboard *board)
{
    for (int i = 0; i < boardWidth; i++)
    {
        if (!board->lanes[i][0])
        {
            return 0;
        }
    }
    board->isWonBy = 3;
    return 1;
}

void updateGameFinishedStatus(struct gameboard *board)
{
    if (board->isWonBy)
    {
        return;
    }

    int columnCount = sizeof(board->lanes) / sizeof(board->lanes[0]);
    int rowCount = sizeof(board->lanes[0]) / sizeof(board->lanes[0][0]);

    if (checkRowsForMatch(columnCount, rowCount, board)
        || checkColumnsForMatch(columnCount, rowCount, board)
        || checkForwardDiagonalsForMatch(columnCount, rowCount, board)
        || checkBackwardDiagonalsForMatch(columnCount, rowCount, board)
        || boardIsFull(board))
    {
    }
}

struct gameboard *put(struct gameboard *board, int laneIndex)
{
    if (board->isWonBy || laneIndex < 0 || laneIndex >= boardWidth)
    {
        return NULL;
    }

    int *updateLane = board->lanes[laneIndex];
    int laneSize =
      sizeof(board->lanes[laneIndex]) / sizeof(board->lanes[laneIndex][0]);
    int rowIndex = 0;

    while (rowIndex < laneSize && updateLane[rowIndex] == 0)
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
