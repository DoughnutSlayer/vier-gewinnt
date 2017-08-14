ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

ifndef CC
	CC = gcc
endif

CFLAGS = -Wall -std=c99 -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = createSequentialTree.h gameboard.h knot.h testHelp.h calculateSequentialWinPercentage.h
OBJS = createSequentialTree.o gameboard.o knot.o testHelp.o calculateSequentialWinPercentage.o testSequentialCalculateWinpercentage.o

testSequentialCalculateWinpercentage : $(OBJS)
	$(CC) $(CFLAGS) -o testSequentialCalculateWinpercentage $(OBJS)

calculateSequentialWinPercentage.o : calculateSequentialWinPercentage.c $(DEPS)
	$(CC) $(CFLAGS) -c calculateSequentialWinPercentage.c

createSequentialTree.o : createSequentialTree.c $(DEPS)
	$(CC) $(CFLAGS) -c createSequentialTree.c

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

knot.o : knot.c $(DEPS)
	$(CC) $(CFLAGS) -c knot.c

testHelp.o : testHelp.c $(DEPS)
	$(CC) $(CFLAGS) -c testHelp.c

testSequentialCalculateWinpercentage.o : testSequentialCalculateWinpercentage.c $(DEPS)
	$(CC) $(CFLAGS) -c testSequentialCalculateWinpercentage.c

.PHONY : all
all :
	make $(OBJS)

.PHONY : clean
clean :
	rm -f $(OBJS) testSequentialCalculateWinpercentage.exe
