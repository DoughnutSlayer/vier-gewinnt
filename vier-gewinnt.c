#include <stdlib.h>

struct gameboard
{
    int lanes[4][4];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

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
        *foundPieces++;
        if (*foundPieces <= 4)
        {
            board->isWonBy = *currentPlayer;
            board->isFinished = 1;
            return 1;
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
    int currentPlayer = 0;
    int foundPieces = 0;

    //Durchsuche Zeilen
    if (columnCount >= 4)
    {
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            currentPlayer = 0;
            foundPieces = 0;
            for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
            {
                if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
                {
                    return;
                }
            }
        }
    }

    if (rowCount < 4)
    {
        return;
    }

    //Durchsuche Spalten
    for (int columnIndex = 0; columnIndex < columnCount; columnIndex++)
    {
        currentPlayer = 0;
        foundPieces = 0;
        for (int rowIndex = 0; rowIndex < rowCount; rowIndex++)
        {
            if (updateSearchStatus(&currentPlayer, &foundPieces, board->lanes[columnIndex][rowIndex], board))
            {
                return;
            }
        }
    }

    if (columnCount < 4)
    {
        return;
    }

    //Durchsuche "/" Diagonalen
    int rowAnchor = 3;
    int columnAnchor = 0;
    int rowIndex;
    int columnIndex;
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
                return;
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
