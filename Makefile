CFLAGS = -I./include

ms : ./src/minesweeper.c ./src/parg.c
	gcc $^ -O3 -o $@ $(CFLAGS)
