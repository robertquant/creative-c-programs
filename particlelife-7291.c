/*
 * particlelife - Terminal Particle Life Simulator
 *
 * Emergent life-like behavior from simple attraction/repulsion rules
 * between colored particle types. Each pair of types has a force coefficient
 * that determines whether they attract or repel, creating mesmerizing
 * self-organizing structures.
 *
 * Controls:
 *   SPACE       - Randomize interaction rules
 *   P           - Toggle pause
 *   R           - Reset particles (keep rules)
 *   +/-         - Increase/decrease particle count
 *   1-6         - Spawn particles of that type at cursor
 *   Arrow keys  - Move cursor
 *   F           - Toggle friction (low/high)
 *   D           - Toggle force display panel
 *   Q/Esc       - Quit
 *
 * Compile:
 *   macOS:  gcc -O2 -D_DARWIN_C_SOURCE -o particlelife particlelife-7291.c -lncurses -lm
 *   Linux:  gcc -O2 -o particlelife particlelife-7291.c -lncurses -lm
 */

#ifdef __APPLE__
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#ifdef __APPLE__
#include <sys/time.h>
#endif

#include <ncurses.h>

/* ── Constants ── */
#define MAX_TYPES       6
#define MAX_PARTICLES   2000
#define DEFAULT_COUNT   600
#define FRICTION        0.05
#define R_MIN           20.0
#define R_MAX           120.0
#define FORCE_SCALE     8.0
#define DT              0.02
#define BOUNCE_DAMP     0.5

/* ── ANSI 256-color palette for particle types ── */
static const int TYPE_COLORS[MAX_TYPES] = {
    196,  /* Red     */
    82,   /* Green   */
    75,   /* Blue    */
    226,  /* Yellow  */
    213,  /* Pink    */
    51,   /* Cyan    */
};

static const char TYPE_CHARS[MAX_TYPES] = {'@', '#', '*', '+', '%', '&'};

/* ── Data structures ── */
typedef struct {
    float x, y;
    float vx, vy;
    int type;
} Particle;

/* ── Global state ── */
static Particle particles[MAX_PARTICLES];
static float rules[MAX_TYPES][MAX_TYPES];
static int num_types = MAX_TYPES;
static int num_particles = DEFAULT_COUNT;
static int paused = 0;
static int show_panel = 1;
static int friction_mode = 0; /* 0=normal, 1=high */
static int cursor_x = 40, cursor_y = 12;
static int W, H; /* terminal dimensions */
static volatile sig_atomic_t running = 1;

/* ── Signal handler ── */
static void on_sigint(int sig) { (void)sig; running = 0; }

/* ── Random float in [-1, 1] ── */
static float randf_range(float lo, float hi) {
    return lo + (hi - lo) * ((float)rand() / (float)RAND_MAX);
}

/* ── Randomize interaction rules ── */
static void randomize_rules(void) {
    for (int i = 0; i < num_types; i++)
        for (int j = 0; j < num_types; j++)
            rules[i][j] = randf_range(-1.0f, 1.0f);
}

/* ── Place particles randomly ── */
static void init_particles(void) {
    int margin = 2;
    for (int i = 0; i < num_particles; i++) {
        particles[i].x = randf_range(margin, W - 2 - margin);
        particles[i].y = randf_range(margin, H - 2 - margin);
        particles[i].vx = 0;
        particles[i].vy = 0;
        particles[i].type = i % num_types;
    }
}

/* ── Force profile: ramp up then ramp down ── */
static inline float force_profile(float r, float attraction) {
    if (r < R_MIN * 0.5f) return attraction * r / (R_MIN * 0.5f) - 1.0f;
    if (r < R_MIN) return attraction * (r - R_MIN) / (R_MIN * 0.5f);
    if (r < R_MAX) return attraction * (1.0f - (r - R_MIN) / (R_MAX - R_MIN));
    return 0.0f;
}

/* ── Physics step ── */
static void update(void) {
    if (paused) return;
    float fric = friction_mode ? 0.15f : FRICTION;

    for (int i = 0; i < num_particles; i++) {
        float fx = 0, fy = 0;
        float px = particles[i].x, py = particles[i].y;
        int ti = particles[i].type;

        for (int j = 0; j < num_particles; j++) {
            if (i == j) continue;
            float dx = particles[j].x - px;
            float dy = particles[j].y - py;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 1.0f) dist = 1.0f;
            if (dist > R_MAX) continue;

            float f = force_profile(dist, rules[ti][particles[j].type]);
            fx += (dx / dist) * f;
            fy += (dy / dist) * f;
        }

        particles[i].vx = (particles[i].vx + fx * FORCE_SCALE * DT) * (1.0f - fric);
        particles[i].vy = (particles[i].vy + fy * FORCE_SCALE * DT) * (1.0f - fric);

        /* Clamp velocity */
        float speed = sqrtf(particles[i].vx * particles[i].vx + particles[i].vy * particles[i].vy);
        if (speed > 30.0f) {
            particles[i].vx *= 30.0f / speed;
            particles[i].vy *= 30.0f / speed;
        }

        particles[i].x += particles[i].vx * DT;
        particles[i].y += particles[i].vy * DT;

        /* Boundary bounce */
        if (particles[i].x < 0)       { particles[i].x = 0;       particles[i].vx *= -BOUNCE_DAMP; }
        if (particles[i].x > W - 1)   { particles[i].x = W - 1;   particles[i].vx *= -BOUNCE_DAMP; }
        if (particles[i].y < 0)       { particles[i].y = 0;       particles[i].vy *= -BOUNCE_DAMP; }
        if (particles[i].y > H - 1)   { particles[i].y = H - 1;   particles[i].vy *= -BOUNCE_DAMP; }
    }
}

/* ── Draw everything ── */
static void draw(void) {
    erase();

    /* Draw particles into a density grid for better visuals */
    int gw = W, gh = H;
    /* Use a simple approach: draw each particle as a colored character */
    for (int i = 0; i < num_particles; i++) {
        int x = (int)particles[i].x;
        int y = (int)particles[i].y;
        if (x < 0 || x >= gw || y < 0 || y >= gh) continue;
        int t = particles[i].type;
        int color_pair = t + 1;
        attron(COLOR_PAIR(color_pair));
        mvaddch(y, x, TYPE_CHARS[t]);
        attroff(COLOR_PAIR(color_pair));
    }

    /* Draw cursor */
    mvaddch(cursor_y, cursor_x, ACS_CKBOARD | A_REVERSE);

    /* HUD */
    attron(A_DIM);
    mvprintw(0, 0, "Particle Life | SPACE:random P:pause R:reset F:friction D:panel +/-:count Q:quit");
    attroff(A_DIM);

    /* Force matrix panel */
    if (show_panel) {
        int px = W - 28;
        int py = 2;
        if (px > 10) {
            attron(A_DIM);
            mvprintw(py, px, "Force Matrix (row->col):");
            attroff(A_DIM);
            /* Header */
            for (int j = 0; j < num_types; j++) {
                attron(COLOR_PAIR(j + 1));
                mvprintw(py + 1, px + 4 + j * 4, "%c", 'A' + j);
                attroff(COLOR_PAIR(j + 1));
            }
            for (int i = 0; i < num_types; i++) {
                attron(COLOR_PAIR(i + 1));
                mvprintw(py + 2 + i, px, "%c:", 'A' + i);
                attroff(COLOR_PAIR(i + 1));
                for (int j = 0; j < num_types; j++) {
                    float v = rules[i][j];
                    int bright = (int)(fabsf(v) * 100);
                    if (bright > 99) bright = 99;
                    if (v > 0.05f) {
                        attron(COLOR_PAIR(j + 1));
                        mvprintw(py + 2 + i, px + 4 + j * 4, "+%d", bright);
                        attroff(COLOR_PAIR(j + 1));
                    } else if (v < -0.05f) {
                        attron(COLOR_PAIR(j + 1));
                        mvprintw(py + 2 + i, px + 4 + j * 4, "-%d", bright);
                        attroff(COLOR_PAIR(j + 1));
                    } else {
                        mvprintw(py + 2 + i, px + 4 + j * 4, " . ");
                    }
                }
            }
        }
    }

    /* Status bar */
    attron(A_DIM);
    mvprintw(H - 1, 0, "N=%d friction=%s %s",
             num_particles,
             friction_mode ? "HIGH" : "LOW",
             paused ? "[PAUSED]" : "");
    attroff(A_DIM);

    refresh();
}

/* ── Spawn particles at cursor ── */
static void spawn_at_cursor(int type, int count) {
    for (int i = 0; i < count && num_particles < MAX_PARTICLES; i++) {
        int idx = num_particles++;
        particles[idx].x = cursor_x + randf_range(-5, 5);
        particles[idx].y = cursor_y + randf_range(-5, 5);
        particles[idx].vx = 0;
        particles[idx].vy = 0;
        particles[idx].type = type;
    }
}

/* ── Change particle count ── */
static void change_count(int delta) {
    int target = num_particles + delta;
    if (target < 10) target = 10;
    if (target > MAX_PARTICLES) target = MAX_PARTICLES;

    if (target > num_particles) {
        /* Add particles */
        for (int i = num_particles; i < target; i++) {
            particles[i].x = randf_range(2, W - 3);
            particles[i].y = randf_range(2, H - 3);
            particles[i].vx = 0;
            particles[i].vy = 0;
            particles[i].type = i % num_types;
        }
    }
    num_particles = target;
}

/* ── Initialize ncurses colors ── */
static void init_colors(void) {
    start_color();
    if (COLORS < 256) {
        /* Fallback: use basic colors */
        int fallback[MAX_TYPES] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE,
                                     COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN };
        for (int i = 0; i < MAX_TYPES; i++)
            init_pair(i + 1, fallback[i], COLOR_BLACK);
    } else {
        for (int i = 0; i < MAX_TYPES; i++)
            init_pair(i + 1, TYPE_COLORS[i], COLOR_BLACK);
    }
}

/* ── Main ── */
int main(void) {
    srand((unsigned)time(NULL));

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    nodelay(stdscr, TRUE);
    mousemask(0, NULL);

    init_colors();
    getmaxyx(stdscr, H, W);

    randomize_rules();
    init_particles();

    cursor_x = W / 2;
    cursor_y = H / 2;

    signal(SIGINT, on_sigint);

    while (running) {
        int ch = getch();
        if (ch == 'q' || ch == 27) break; /* 27 = Esc */
        switch (ch) {
            case ' ': randomize_rules(); break;
            case 'p': paused = !paused; break;
            case 'r': init_particles(); break;
            case 'f': friction_mode = !friction_mode; break;
            case 'd': show_panel = !show_panel; break;
            case '+': case '=': change_count(50); break;
            case '-': case '_': change_count(-50); break;
            case '1': spawn_at_cursor(0, 10); break;
            case '2': spawn_at_cursor(1, 10); break;
            case '3': spawn_at_cursor(2, 10); break;
            case '4': spawn_at_cursor(3, 10); break;
            case '5': spawn_at_cursor(4, 10); break;
            case '6': spawn_at_cursor(5, 10); break;
            case KEY_UP:    cursor_y = (cursor_y > 0) ? cursor_y - 1 : 0; break;
            case KEY_DOWN:  cursor_y = (cursor_y < H-1) ? cursor_y + 1 : H-1; break;
            case KEY_LEFT:  cursor_x = (cursor_x > 0) ? cursor_x - 1 : 0; break;
            case KEY_RIGHT: cursor_x = (cursor_x < W-1) ? cursor_x + 1 : W-1; break;
        }

        update();
        draw();

        /* Handle terminal resize */
        int nw, nh;
        getmaxyx(stdscr, nh, nw);
        if (nw != W || nh != H) {
            W = nw; H = nh;
            if (cursor_x >= W) cursor_x = W - 1;
            if (cursor_y >= H) cursor_y = H - 1;
        }

        napms(16); /* ~60 FPS target */
    }

    endwin();
    return 0;
}
