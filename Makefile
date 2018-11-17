CFLAGS = -I./include

ms : ./src/minesweeper.c ./src/parg.c
	gcc $^ -O3 -o $@.out $(CFLAGS)

ms_debug : ./src/minesweeper.c ./src/parg.c
	gcc $^ -g -o $@.out $(CFLAGS)
