OBJS = gameboard.o

gameboard.o : gameboard.c gameboard.h
	gcc -c gameboard.c

.PHONY : clean
clean :
	rm -f $(OBJS)
