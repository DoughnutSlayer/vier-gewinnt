struct gameboard
{
    int lanes[4][4];
    int isWonBy;
    int isFinished;
    int nextPlayer;
};

struct gameboard put(struct gameboard current, int laneIndex)
{
    struct gameboard new = current;
    int *updateLane = new.lanes[laneIndex];
    int laneSize = sizeof(updateLane)/sizeof(updateLane[0]);
    int rowIndex = 0;

    while(rowIndex < laneSize && updateLane[rowIndex] == 0)
    {
        rowIndex += 1;
    }

    if (rowIndex > 0)
    {
        updateLane[rowIndex - 1] = new.nextPlayer;
        new.nextPlayer = 2 - (2 % new.nextPlayer);
        return new;
    }
    return NULL;
}
