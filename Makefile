CFLAGS = -I./include

ms : ./src/minesweeper.c ./src/parg.c
	gcc $^ -O3 -o $@ $(CFLAGS)

ms_unix : ./src/minesweeper.c ./src/parg.c
	gcc $^ -O3 -o ms.out $(CFLAGS)

ms_debug : ./src/minesweeper.c ./src/parg.c
	gcc $^ -g -o $@ $(CFLAGS)
