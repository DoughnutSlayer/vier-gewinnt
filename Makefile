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
DEPS = gameboard.h knot.h
OBJS = gameboard.o knot.o

gameboard.o : gameboard.c $(DEPS)
	$(CC) $(CFLAGS) -c gameboard.c

knot.o : knot.c $(DEPS)
	$(CC) $(CFLAGS) -c knot.c

.PHONY : clean
clean :
	rm -f $(OBJS)
