#include <stdlib.h>
#include "gameboard.h"

int updateSearchStatus(int *currentPlayer, int *foundPieces, int currentPiece, struct gameboard *board)
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
            board->isFinished = 1;
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
            if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
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
            if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
            {
                return 1;
            }
        }
    }
    return 0;
}

int checkForwardDiagonalsForMatch(int columnCount, int rowCount, struct gameboard *board)
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
            if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
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

int checkBackwardDiagonalsForMatch(int columnCount, int rowCount, struct gameboard *board)
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
            if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
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

void updateGameFinishedStatus(struct gameboard *board)
{
    if (board->isFinished)
    {
        return;
    }

    int columnCount = sizeof(board->lanes)/sizeof(board->lanes[0]);
    int rowCount = sizeof(board->lanes[0])/sizeof(board->lanes[0][0]);

    if (checkRowsForMatch(columnCount, rowCount, board)
        || checkColumnsForMatch(columnCount, rowCount, board)
        || checkForwardDiagonalsForMatch(columnCount, rowCount, board)
        || checkBackwardDiagonalsForMatch(columnCount, rowCount, board))
    {
    }
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
