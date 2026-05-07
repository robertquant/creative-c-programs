/*
 * plasma-2243.c - Terminal Plasma Effect Generator
 *
 * Mesmerizing real-time animated plasma / interference patterns
 * rendered with ANSI 256-color terminal graphics.  Five distinct
 * plasma presets and five color palettes to explore.
 *
 * Compile (both platforms):
 *   macOS:  gcc -std=gnu11 -Wall -O2 -o plasma plasma-2243.c -lm
 *   Linux:  gcc -std=gnu11 -Wall -O2 -o plasma plasma-2243.c -lm
 *
 * Controls
 *   1-5      Switch plasma pattern
 *   c / C    Cycle palette forward / backward
 *   + / -    Adjust animation speed
 *   Space    Pause / Resume
 *   f        Toggle FPS display
 *   h        Toggle help overlay
 *   q / Esc  Quit
 */

/* ── Platform compat ─────────────────────────────────────────────── */
#ifdef __APPLE__
#  ifndef _DARWIN_C_SOURCE
#    define _DARWIN_C_SOURCE
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

/* ── Constants ───────────────────────────────────────────────────── */
#define N_PAT   5                /* plasma patterns   */
#define N_PAL   5                /* color palettes    */
#define SBITS   12               /* sin-table bits    */
#define SSIZE   (1 << SBITS)
#define SMASK   (SSIZE - 1)
#define TGT_FPS 25

/* ── Types ───────────────────────────────────────────────────────── */
typedef struct { double r, g, b; } RGB;
typedef double (*plasma_fn)(double, double, double);
typedef int    (*palette_fn)(double);

/* ── Globals ─────────────────────────────────────────────────────── */
static volatile sig_atomic_t g_run = 1, g_resize = 1;
static int  g_w = 80, g_h = 24;
static double g_sin[SSIZE];                       /* sin lookup table */
static int    g_plut[N_PAL][256];                 /* palette LUT      */
static struct termios g_orig;                     /* saved termios    */
static char *g_fb = NULL;                         /* frame buffer     */
static int   g_fbsz = 0;

/* ── Sin table ───────────────────────────────────────────────────── */
static void init_sin(void)
{
    for (int i = 0; i < SSIZE; i++)
        g_sin[i] = sin(2.0 * M_PI * i / SSIZE);
}

static inline double fsin(double x)
{
    x = fmod(x, 2.0 * M_PI);
    if (x < 0) x += 2.0 * M_PI;
    return g_sin[(int)(x * (SSIZE / (2.0 * M_PI))) & SMASK];
}

/* ── Terminal helpers ────────────────────────────────────────────── */
static void on_int(int s)   { (void)s; g_run = 0; }
static void on_winch(int s) { (void)s; g_resize = 1; }

static void get_size(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        g_w = ws.ws_col;
        g_h = ws.ws_row;
    }
}

static void raw_on(void)
{
    tcgetattr(STDIN_FILENO, &g_orig);
    struct termios t = g_orig;
    t.c_iflag &= ~(ICRNL | IXON);
    t.c_lflag &= ~(ECHO | ICANON);
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);
}

static void raw_off(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig); }

static double now_s(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

/* ── Color math ──────────────────────────────────────────────────── */
static RGB lerp_c(RGB a, RGB b, double t)
{
    return (RGB){ a.r + (b.r - a.r) * t,
                  a.g + (b.g - a.g) * t,
                  a.b + (b.b - a.b) * t };
}

static RGB grad(double t, const RGB *s, int n)
{
    t -= floor(t);
    double f = t * (n - 1);
    int i = (int)f;
    if (i >= n - 1) return s[n - 1];
    return lerp_c(s[i], s[i + 1], f - i);
}

static RGB hsv2rgb(double h, double s, double v)
{
    double c = v * s, x = c * (1 - fabs(fmod(h / 60.0, 2) - 1)), m = v - c;
    RGB r;
    if      (h < 60)  r = (RGB){c, x, 0};
    else if (h < 120) r = (RGB){x, c, 0};
    else if (h < 180) r = (RGB){0, c, x};
    else if (h < 240) r = (RGB){0, x, c};
    else if (h < 300) r = (RGB){x, 0, c};
    else              r = (RGB){c, 0, x};
    r.r += m; r.g += m; r.b += m;
    return r;
}

/* Map [0,1] RGB to ANSI 256-color (6x6x6 cube) */
static int to256(double r, double g, double b)
{
    int ri = (int)(r * 5 + .5), gi = (int)(g * 5 + .5), bi = (int)(b * 5 + .5);
    if (ri < 0) ri = 0; else if (ri > 5) ri = 5;
    if (gi < 0) gi = 0; else if (gi > 5) gi = 5;
    if (bi < 0) bi = 0; else if (bi > 5) bi = 5;
    return 16 + ri * 36 + gi * 6 + bi;
}

/* ── Palettes ────────────────────────────────────────────────────── */
static int pal_fire(double v)
{
    static const RGB s[] = {{0,0,0},{.5,0,0},{1,.2,0},{1,.6,0},{1,1,.3},{1,1,1}};
    RGB c = grad(v, s, 6); return to256(c.r, c.g, c.b);
}

static int pal_ocean(double v)
{
    static const RGB s[] = {{0,0,.1},{0,.15,.45},{0,.45,.8},{0,.8,.9},{.5,1,1},{1,1,1}};
    RGB c = grad(v, s, 6); return to256(c.r, c.g, c.b);
}

static int pal_neon(double v)
{
    static const RGB s[] = {{0,0,0},{.5,0,.9},{0,.8,1},{0,1,.4},{1,1,0},{1,1,1}};
    RGB c = grad(v, s, 6); return to256(c.r, c.g, c.b);
}

static int pal_rainbow(double v)
{
    RGB c = hsv2rgb(fmod(v, 1) * 360, .9, .95);
    return to256(c.r, c.g, c.b);
}

static int pal_twilight(double v)
{
    static const RGB s[] = {{.05,0,.15},{.2,0,.45},{.55,.1,.55},{.85,.2,.4},{1,.55,.2},{1,.85,.35}};
    RGB c = grad(v, s, 6); return to256(c.r, c.g, c.b);
}

static const palette_fn g_pal[N_PAL] = {
    pal_fire, pal_ocean, pal_neon, pal_rainbow, pal_twilight
};
static const char *g_pname[N_PAL] = {
    "Fire", "Ocean", "Neon", "Rainbow", "Twilight"
};

static void build_lut(void)
{
    for (int p = 0; p < N_PAL; p++)
        for (int i = 0; i < 256; i++)
            g_plut[p][i] = g_pal[p](i / 255.0);
}

/* ── Plasma patterns ─────────────────────────────────────────────── */
static double pl_classic(double x, double y, double t)
{
    return fsin(x * .05 + t)
         + fsin(y * .05 + t * .7)
         + fsin((x + y) * .03 + t * .5)
         + fsin(sqrt(x * x + y * y) * .04 + t * 1.3);
}

static double pl_spiral(double x, double y, double t)
{
    double cx = x * .5, cy = y * .5;
    double r = sqrt(cx * cx + cy * cy);
    double a = atan2(cy, cx);
    return fsin(r * .1 - a * 2 + t)
         + fsin(r * .05 + t * .5)
         + fsin(a * 3 + t * .7) * .5;
}

static double pl_diamond(double x, double y, double t)
{
    double ax = fabs(x), ay = fabs(y);
    return fsin((ax + ay) * .06 + t)
         + fsin(x * .04 + t * .8)
         + fsin(y * .04 - t * .6)
         + fsin((ax - ay) * .05 + t * 1.2);
}

static double pl_ripple(double x, double y, double t)
{
    double r = sqrt(x * x * .25 + y * y * .25);
    return fsin(r * .15 + t) * cos(r * .1 - t * .5)
         + fsin(x * .03 + t * .3)
         + fsin(y * .04 - t * .4);
}

static double pl_warp(double x, double y, double t)
{
    double sx = fsin(t * .3) * 30, sy = cos(t * .4) * 20;
    return fsin((x + sx) * .04 + t * .5)
         + fsin((y + sy) * .04 + t * .3)
         + fsin(sqrt((x+sx)*(x+sx) + (y+sy)*(y+sy)) * .03 + t)
         + fsin((x - sy) * .05 + (y + sx) * .05 + t * .7);
}

static const plasma_fn g_plfn[N_PAT] = {
    pl_classic, pl_spiral, pl_diamond, pl_ripple, pl_warp
};
static const char *g_plname[N_PAT] = {
    "Classic", "Spiral", "Diamond", "Ripple", "Warp"
};

/* ── Rendering ───────────────────────────────────────────────────── */
static int ensure_fb(int need)
{
    if (g_fbsz >= need) return 0;
    free(g_fb);
    g_fb = malloc(need);
    if (!g_fb) { g_fbsz = 0; return -1; }
    g_fbsz = need;
    return 0;
}

static void render_plasma(int w, int h, int pat, int pal, double t)
{
    int need = w * h * 16 + 4096;
    if (ensure_fb(need)) return;

    plasma_fn pf = g_plfn[pat];
    int *lut = g_plut[pal];
    int p = 0;

    /* cursor home */
    p += sprintf(g_fb + p, "\033[H");

    for (int y = 0; y < h - 1; y++) {
        int prev = -1;
        for (int x = 0; x < w; x++) {
            double v = pf((double)x, (double)y, t);
            /* normalize from [-4,4] to [0,255] */
            int iv = (int)((v + 4.0) * 31.875);
            if (iv < 0)   iv = 0;
            if (iv > 255) iv = 255;
            int c = lut[iv];
            if (c != prev) {
                p += sprintf(g_fb + p, "\033[48;5;%dm", c);
                prev = c;
            }
            g_fb[p++] = ' ';
        }
        p += sprintf(g_fb + p, "\033[0m\n");
    }

    ssize_t wr __attribute__((unused)) = write(STDOUT_FILENO, g_fb, p);
}

static void render_status(int w, int pat, int pal, double speed,
                           int paused, int show_fps, double fps)
{
    char bar[512];
    const char *st = paused ? "||" : ">>";
    int sl;

    sl = sprintf(bar, " %s P%d:%-8s  C%d:%-10s  Spd:%.1fx",
                 st, pat + 1, g_plname[pat],
                 pal + 1, g_pname[pal], speed);
    if (show_fps) sl += sprintf(bar + sl, "  %.0ffps", fps);

    /* pick a hint that fits */
    const char *hint = (w < 90)
        ? " [h]elp [q]uit"
        : " [1-5]pattern [c]olor [+/-]speed [h]elp [q]uit";
    sl += sprintf(bar + sl, "%s", hint);

    printf("\033[48;5;236;38;5;255m%s", bar);
    int pad = w - sl;
    if (pad > 0) for (int i = 0; i < pad; i++) putchar(' ');
    printf("\033[0m");
    fflush(stdout);
}

static void render_help(int w, int h)
{
    const char *lines[] = {
        "  Terminal Plasma Effect Generator  ",
        "  --------------------------------  ",
        "  [1-5]    Switch plasma pattern    ",
        "  [c/C]    Cycle palette fwd/back   ",
        "  [+/-]    Speed up / slow down     ",
        "  [Space]  Pause / Resume           ",
        "  [f]      Toggle FPS display       ",
        "  [h]      Toggle this help         ",
        "  [q/Esc]  Quit                     ",
        "  --------------------------------  ",
        "  Press any key to dismiss...       ",
    };
    int bw = 36, bh = 11;
    int bx = (w - bw) / 2; if (bx < 1) bx = 1;
    int by = (h - bh) / 2; if (by < 1) by = 1;

    for (int i = 0; i < bh && (by + i) < h; i++) {
        printf("\033[%d;%dH\033[48;5;55;38;5;255m%s\033[0m",
               by + i, bx, lines[i]);
    }
    fflush(stdout);
}

/* ── Entry point ─────────────────────────────────────────────────── */
int main(void)
{
    init_sin();
    build_lut();

    signal(SIGINT,  on_int);
    signal(SIGWINCH, on_winch);
    atexit(raw_off);

    raw_on();
    get_size();

    printf("\033[?25l\033[2J");   /* hide cursor, clear screen */
    fflush(stdout);

    int pat = 0, pal = 0, paused = 0, show_fps = 0;
    int show_help = 1;             /* show help on startup */
    double speed = 1.0, t = 0.0;
    double fps = 0, t0 = now_s(), t_last = t0;
    int frames = 0;

    while (g_run) {
        if (g_resize) {
            get_size();
            g_resize = 0;
            printf("\033[2J");
        }

        int w = g_w, h = g_h;

        /* render plasma field */
        render_plasma(w, h, pat, pal, t);
        /* status bar */
        render_status(w, pat, pal, speed, paused, show_fps, fps);
        /* help overlay (auto-dismiss after 5 s) */
        if (show_help) {
            static double help_t0 = 0;
            if (help_t0 == 0) help_t0 = now_s();
            if (now_s() - help_t0 < 5.0)
                render_help(w, h);
            else
                show_help = 0;
        }

        /* keyboard input */
        char c;
        while (read(STDIN_FILENO, &c, 1) > 0) {
            /* dismiss help on any key */
            if (show_help && c != 'h' && c != 'H') {
                show_help = 0;
                continue;
            }
            switch (c) {
            case 'q': case 'Q': g_run = 0; break;
            case 27: {                       /* Escape or CSI sequence */
                char e;
                if (read(STDIN_FILENO, &e, 1) <= 0)
                    g_run = 0;               /* plain Esc -> quit      */
                else {
                    /* consume rest of CSI (arrow keys etc.) */
                    if (e == '[') {
                        struct timespec w10 = {0, 10000000};
                        while (read(STDIN_FILENO, &e, 1) > 0)
                            if (e >= '@' && e <= '~') break;
                        nanosleep(&w10, NULL);
                    }
                }
                break;
            }
            case '1': case '2': case '3': case '4': case '5':
                pat = c - '1'; break;
            case 'c': pal = (pal + 1) % N_PAL; break;
            case 'C': pal = (pal - 1 + N_PAL) % N_PAL; break;
            case '+': case '=':
                speed = speed < 5.0 ? speed + 0.5 : 5.0; break;
            case '-': case '_':
                speed = speed > 0.5 ? speed - 0.5 : 0.5; break;
            case ' ':
                paused = !paused; break;
            case 'f': case 'F':
                show_fps = !show_fps; break;
            case 'h': case 'H':
                show_help = !show_help; break;
            default: break;
            }
        }

        /* advance time */
        if (!paused) t += 0.05 * speed;

        /* FPS counter (update every 0.5 s) */
        frames++;
        double tn = now_s();
        if (tn - t_last >= 0.5) {
            fps = frames / (tn - t_last);
            frames = 0;
            t_last = tn;
        }

        /* frame-rate limiter */
        double elapsed = now_s() - t0;
        double target = 1.0 / TGT_FPS;
        if (elapsed < target) {
            struct timespec ts;
            double rem = target - elapsed;
            ts.tv_sec  = (time_t)rem;
            ts.tv_nsec = (long)((rem - ts.tv_sec) * 1e9);
            nanosleep(&ts, NULL);
        }
        t0 = now_s();
    }

    /* cleanup */
    printf("\033[?25h\033[0m\033[2J");
    printf("Thanks for watching the plasma!\n");
    fflush(stdout);
    free(g_fb);
    return 0;
}
