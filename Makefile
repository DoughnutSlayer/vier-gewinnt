ifndef BOARD_WIDTH
	BOARD_WIDTH = 4
endif

ifndef BOARD_HEIGHT
	BOARD_HEIGHT = 4
endif

CC = mpicc

CFLAGS = -Wall -g -std=c99 -DBOARD_WIDTH=$(BOARD_WIDTH) -DBOARD_HEIGHT=$(BOARD_HEIGHT)
DEPS = createParallelTree.h gameboard.h knot.h
OBJS = createParallelTree.o gameboard.o vier-gewinnt.o

vier-gewinnt : $(OBJS)
	$(CC) $(CFLAGS) -lm -o vier-gewinnt $(OBJS)

createParallelTree.o : createParallelTree.c $(DEPS)
	$(CC) $(CFLAGS) -c createParallelTree.c

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

vier-gewinnt.o : vier-gewinnt.c $(DEPS)
	$(CC) $(CFLAGS) -c vier-gewinnt.c

.PHONY : all
all :
	make $(OBJS)

.PHONY : clean
clean :
	rm -f $(OBJS) '.gameboardsA' '.gameboardsB' vier-gewinnt vier-gewinnt.exe
