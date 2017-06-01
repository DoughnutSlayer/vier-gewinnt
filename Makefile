OBJS = gameboard.o

gameboard.o : gameboard.c
	gcc -c gameboard.c

.PHONY : clean
clean :
	rm -f $(OBJS)
