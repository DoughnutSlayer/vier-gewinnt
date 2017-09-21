#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include "createParallelTree.h"
#include "gameboard.h"
#include "knot.h"

extern const int boardWidth, boardHeight;

char invalidInputMessage[42];

struct knot playerKnot;

void printPlayerPrompt()
{
    printf(" ");
    for (int i = 0; i < boardWidth; i++)
    {
        printf(" %d  ", i);
    }
    printf("\n");

    printf("┌───");
    for (int i = 1; i < boardWidth; i++)
    {
        printf("┬───");
    }
    printf("┐\n");

    for (int rowIndex = 0; rowIndex < boardHeight; rowIndex++)
    {
        for (int columnIndex = 0; columnIndex < boardWidth; columnIndex++)
        {
            char boardPiece = ' ';
            if (playerKnot.gameboard->lanes[columnIndex][rowIndex] == 1)
            {
                boardPiece = 'O';
            }
            else if (playerKnot.gameboard->lanes[columnIndex][rowIndex] == 2)
            {
                boardPiece = 'X';
            }
            printf("│ %c ", boardPiece);
        }
        printf("│\n");

        if (rowIndex == boardHeight - 1)
        {
            break;
        }
        printf("├───");
        for (int columnIndex = 1; columnIndex < boardWidth; columnIndex++)
        {
            printf("┼───");
        }
        printf("┤\n");
    }

    printf("└───");
    for (int i = 1; i < boardWidth; i++)
    {
        printf("┴───");
    }
    printf("┘\n");
    printf("Enter where to put the next 'O': ");
}

int getNumberInput()
{
    int validInput = 0;
    int input;
    validInput = scanf("%d", &input);
    while (validInput <= 0)
    {
        printf("%s", invalidInputMessage);
        fflush(NULL);
        validInput = scanf("%d", &input);
    }
    return input;
}

int main(int argc, char *argv[])
{
    sprintf(&invalidInputMessage, "Please enter a number between 0 and %d: ", boardWidth - 1);

    struct gameboard emptyBoard = {0};
    emptyBoard.nextPlayer = 1;
    playerKnot.gameboard = &emptyBoard;
    calculateHash(&playerKnot);

    MPI_Init(&argc, &argv);

    buildParallelTree(root);

    MPI_Finalize();
    return 0;
}
