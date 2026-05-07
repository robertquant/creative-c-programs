# Creative C Programs

Auto-generated collection of creative C programs, built entirely by AI (Claude Code).

Each program is self-contained, compiles with standard `gcc`, and runs in the terminal.

## Programs (33 total)

### Games
| # | Program | Description |
|---|---------|-------------|
| 1 | `fireworks-9040.c` | Terminal Fireworks Particle Simulator |
| 2 | `matrixrain-1923.c` | Terminal Matrix Digital Rain Effect |
| 3 | `minesweeper-8579.c` | Terminal Minesweeper (3 difficulties, flood-fill, flagging) |
| 4 | `roguelike-3152.c` | Terminal Roguelike Dungeon Crawler (10 floors, combat, inventory) |
| 5 | `tetris-5596.c` | Terminal Tetris (SRS rotation, ghost piece, hold, 7-bag randomizer) |
| 6 | `breakout-9767.c` | Terminal Breakout/Arkanoid (5 levels, 4 power-ups, particle effects) |
| 7 | `g2048-2710.c` | Terminal 2048 Puzzle (smooth animations, undo, continue-after-win) |
| 8 | `snake-4324.c` | Terminal Snake with AI (BFS pathfinding, human/AI toggle) |
| 9 | `poker-1501.c` | Terminal Texas Hold'em Poker (3 AI opponents, full hand evaluation) |
| 10 | `connect4-6475.c` | Terminal Connect Four with AI (minimax alpha-beta, 3 difficulties) |
| 11 | `chess-2947.c` | Terminal Chess with AI (minimax depth 4, full rules, undo) |
| 12 | `towerdef-7537.c` | Terminal Tower Defense (4 tower types, 4 enemy types, wave system) |
| 13 | `sudoku-2765.c` | Terminal Sudoku (3 difficulties, pencil marks, auto-solve animation) |

### Simulations
| # | Program | Description |
|---|---------|-------------|
| 14 | `boids-1507.c` | Boids Flocking Simulation (Craig Reynolds algorithm, predator avoidance) |
| 15 | `lifesim-9490.c` | Cellular Automata Explorer (7 rulesets, 8 stamp patterns) |
| 16 | `fluidsim-18879.c` | Fluid Dynamics Simulator (Navier-Stokes stable fluids) |
| 17 | `nbody-4199.c` | N-Body Gravity Simulator (4 presets, orbital trails, collision merging) |
| 18 | `clothsim-9722.c` | Verlet Cloth Physics Simulation (tearing, wind, interactive cursor) |
| 19 | `reactdiff-5841.c` | Reaction-Diffusion Simulator (Gray-Scott model, 6 presets) |
| 20 | `fallsand-9948.c` | Falling Sand Particle Simulation (10 materials, chemical interactions) |
| 21 | `solarium-5087.c` | Solar System Simulator (Kepler orbital mechanics, 8 planets) |

### Graphics & Rendering
| # | Program | Description |
|---|---------|-------------|
| 22 | `wireframe3d-8755.c` | 3D ASCII Wireframe Renderer (6 shapes, depth shading) |
| 23 | `mandelbrot-4468.c` | Interactive Mandelbrot/Julia Set Explorer (7 palettes, smooth zoom) |
| 24 | `voxelspace-8172.c` | Voxel Space Terrain Flythrough (procedural island, minimap) |
| 25 | `raycaster-5013.c` | Pseudo-3D Raycasting Engine (DDA, ANSI 256-color, minimap) |
| 26 | `lsystem-9296.c` | L-System Fractal Generator (10 fractals, animation) |

### Algorithms & Visualization
| # | Program | Description |
|---|---------|-------------|
| 27 | `sortviz-1082.c` | Sorting Algorithm Visualizer (6 algorithms, animated bars) |
| 28 | `pathfind-1428.c` | Pathfinding Visualizer (BFS, DFS, Dijkstra, A*) |
| 29 | `mazesolve-7740.c` | Maze Generator & Solver (3 gen algos, 4 solve algos) |

### Tools
| # | Program | Description |
|---|---------|-------------|
| 30 | `enigma-3028.c` | Enigma Cipher Machine Simulator (historically accurate rotors) |
| 31 | `synth-7487.c` | Chiptune Synthesizer (4 waveforms, piano keyboard, WAV export) |
| 32 | `spreadsheet-6339.c` | Terminal Spreadsheet (formula engine, cell references, curses UI) |
| 33 | `httpd-4295.c` | Tiny HTTP Server (HTTP/1.1, MIME types, range requests, ncurses dashboard) |

## How to Compile

Each `.c` file contains compile instructions in the header comments. In general:

```bash
# Most programs compile with just gcc
gcc -o program program-name-XXXX.c -lm -lncurses

# Some may need additional flags
gcc -o program program-name-XXXX.c -lm -lncurses -lpthread
```

Or use the included Makefile:

```bash
make
```

## How to Run

```bash
./program-name
```

Most programs use terminal (ncurses/ANSI) rendering. No external GUI required.

## Auto-Generation

These programs are automatically generated every 20 minutes by an AI agent powered by Claude Code via [cc-connect](https://github.com/chenhg5/cc-connect). Each program has a unique theme, tracked in `themes.log` to avoid repetition.

## License

MIT
