CC      = gcc
CFLAGS  = -std=gnu11 -Wall -O2

# macOS compatibility
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -D_DARWIN_C_SOURCE
	NCURSES_LIB = -lncurses
else
	NCURSES_LIB = -lncursesw
endif

TARGETS = fireworks matrixrain mandelbrot voxelspace enigma sortviz \
          minesweeper roguelike tetris pathfind fallsand raycaster chess \
          boids lifesim lsystem fluidsim synth mazesolve nbody breakout \
          spreadsheet towerdef clothsim sudoku g2048 snake poker \
          reactdiff connect4 httpd solarium wireframe3d

all: $(TARGETS)

fireworks: fireworks-9040.c
	$(CC) $(CFLAGS) -o $@ $< -lm

matrixrain: matrixrain-1923.c
	$(CC) $(CFLAGS) -o $@ $< -lm

mandelbrot: mandelbrot-4468.c
	$(CC) $(CFLAGS) -o $@ $< -lm

voxelspace: voxelspace-8172.c
	$(CC) $(CFLAGS) -o $@ $< -lm

enigma: enigma-3028.c
	$(CC) $(CFLAGS) -o $@ $<

sortviz: sortviz-1082.c
	$(CC) $(CFLAGS) -o $@ $< -lm

minesweeper: minesweeper-8579.c
	$(CC) $(CFLAGS) -o $@ $< -lm

roguelike: roguelike-3152.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

tetris: tetris-5596.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

pathfind: pathfind-1428.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

fallsand: fallsand-9948.c
	$(CC) $(CFLAGS) -o $@ $< -lm

raycaster: raycaster-5013.c
	$(CC) $(CFLAGS) -o $@ $< -lm

chess: chess-2947.c
	$(CC) $(CFLAGS) -o $@ $< -lm

boids: boids-1507.c
	$(CC) $(CFLAGS) -o $@ $< -lm

lifesim: lifesim-9490.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

lsystem: lsystem-9296.c
	$(CC) $(CFLAGS) -o $@ $< -lm

fluidsim: fluidsim-18879.c
	$(CC) $(CFLAGS) -o $@ $< -lm

synth: synth-7487.c
	$(CC) $(CFLAGS) -o $@ $< -lm

mazesolve: mazesolve-7740.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

nbody: nbody-4199.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

breakout: breakout-9767.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

spreadsheet: spreadsheet-6339.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

towerdef: towerdef-7537.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

clothsim: clothsim-9722.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

sudoku: sudoku-2765.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

g2048: g2048-2710.c
	$(CC) $(CFLAGS) -o $@ $< -lm $(NCURSES_LIB)

snake: snake-4324.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

poker: poker-1501.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

reactdiff: reactdiff-5841.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

connect4: connect4-6475.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

httpd: httpd-4295.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses -lpthread

solarium: solarium-5087.c
	$(CC) $(CFLAGS) -o $@ $< -lm -lncurses

wireframe3d: wireframe3d-8755.c
	$(CC) $(CFLAGS) -o $@ $< -lm

clean:
	rm -f $(TARGETS)

.PHONY: all clean
