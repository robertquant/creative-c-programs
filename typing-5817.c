/*
 * typing-5817.c - Terminal Typing Speed Test
 *
 * A feature-rich typing speed test running entirely in the terminal.
 * Test your typing speed with real-time WPM tracking, accuracy
 * measurement, and character-by-character visual feedback.
 *
 * Features:
 *   - 24 text passages across 4 categories (Common, Code, Quotes, Mixed)
 *   - 4 time modes: 15s, 30s, 60s, 120s
 *   - Real-time WPM and accuracy display
 *   - Color-coded character feedback (green=correct, red=error)
 *   - Word-wrapped text display with scroll
 *   - Progress bar with time remaining
 *   - Results screen with problem-key analysis
 *   - Session best score tracking
 *   - Retry and menu navigation
 *
 * Compile:
 *   macOS: gcc -std=gnu11 -Wall -O2 -D_DARWIN_C_SOURCE -o typing typing-5817.c -lncurses -lm
 *   Linux: gcc -std=gnu11 -Wall -O2 -o typing typing-5817.c -lncurses -lm
 *
 * Run: ./typing
 *
 * Requires: ncurses, libm (math)
 */

#ifdef __APPLE__
#  ifndef _DARWIN_C_SOURCE
#    define _DARWIN_C_SOURCE
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>

#include <ncurses.h>

/* ── Constants ─────────────────────────────────────────────────────────── */

#define MAX_TEXT      4096
#define NUM_PASSAGES  24
#define NUM_CATS      5
#define NUM_DURS      4
#define MAX_LINES     64
#define MAX_PROBLEMS  5

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/* Color pair indices */
enum {
    CP_DEFAULT = 0,
    CP_CORRECT,    /* green  - correctly typed char */
    CP_WRONG,      /* red    - incorrectly typed char */
    CP_PENDING,    /* white  - not yet typed (dimmed) */
    CP_CURSOR,     /* black on white - current position */
    CP_HEADER,     /* cyan   - header bar */
    CP_ACCENT,     /* yellow - highlights */
    CP_DIM,        /* white  - de-emphasized */
    CP_BAR_FILL,   /* green  - progress bar filled */
    CP_BAR_EMPTY,  /* white  - progress bar empty (dim) */
};

static const int    DURS[NUM_DURS]        = {15, 30, 60, 120};
static const char  *DUR_LABELS[NUM_DURS]  = {"15s", "30s", "60s", "120s"};
static const char  *CAT_LABELS[NUM_CATS]  = {
    "All", "Common", "Code", "Quotes", "Mixed"
};
/* Map category selector to passage filter: -1 = all */
static const int    CAT_FILTERS[NUM_CATS] = {-1, 0, 1, 2, 3};

/* ── Text Passages ─────────────────────────────────────────────────────── */

typedef struct { int cat; const char *text; } Passage;

static const Passage passages[NUM_PASSAGES] = {
    /* 0 - Common English */
    {0, "the quick brown fox jumps over the lazy dog near the river bank while "
        "the sun sets behind the mountains creating a beautiful golden glow "
        "across the valley floor below the distant clouds"},
    {0, "she opened the old wooden door and stepped outside into the warm morning "
        "air the birds were singing their sweet songs and the sky was a brilliant "
        "shade of blue stretching far above the rolling green hills"},
    {0, "he walked slowly through the ancient library running his fingers along "
        "the dusty spines of countless books each one holding a vast world of "
        "stories waiting to be discovered by someone new and curious"},
    {0, "they sat around the crackling campfire telling stories and roasting "
        "marshmallows while the stars appeared one by one in the darkening sky "
        "above the tall pine trees that surrounded their small campsite"},

    /* 1 - Programming */
    {1, "for (int i = 0; i < n; i++) { if (arr[i] > max_val) { max_val = arr[i]; "
        "idx = i; } } return idx;"},
    {1, "struct Node { int data; struct Node *next; }; "
        "Node *insert(Node *h, int v) { Node *n = malloc(sizeof(Node)); "
        "n->data = v; n->next = h; return n; }"},
    {1, "void quicksort(int *a, int lo, int hi) { if (lo < hi) { "
        "int p = partition(a, lo, hi); quicksort(a, lo, p - 1); "
        "quicksort(a, p + 1, hi); } }"},
    {1, "typedef struct { double x, y; } Point; "
        "double dist(Point a, Point b) { return sqrt((a.x-b.x)*(a.x-b.x) "
        "+ (a.y-b.y)*(a.y-b.y)); }"},
    {1, "int bsearch(int *a, int n, int t) { int lo = 0, hi = n - 1; "
        "while (lo <= hi) { int m = lo + (hi - lo) / 2; "
        "if (a[m] == t) return m; if (a[m] < t) lo = m + 1; "
        "else hi = m - 1; } return -1; }"},
    {1, "char *reverse(char *s) { int len = strlen(s); "
        "for (int i = 0; i < len / 2; i++) { char t = s[i]; "
        "s[i] = s[len - 1 - i]; s[len - 1 - i] = t; } return s; }"},

    /* 2 - Quotes */
    {2, "The only way to do great work is to love what you do. If you have not "
        "found it yet keep looking. Do not settle. As with all matters of the "
        "heart you will know when you find it."},
    {2, "In the middle of difficulty lies opportunity. Life is what happens when "
        "you are busy making other plans. The greatest glory in living lies not "
        "in never falling but in rising every time we fall."},
    {2, "It does not do to dwell on dreams and forget to live. Happiness can be "
        "found even in the darkest of times if one only remembers to turn on the "
        "light and look for the good that remains around us all."},
    {2, "The best time to plant a tree was twenty years ago. The second best "
        "time is now. Do not judge each day by the harvest you reap but by the "
        "seeds that you plant for the future ahead of you."},

    /* 3 - Mixed / Technical */
    {3, "The five boxing wizards jump quickly at dawn. Pack my box with five "
        "dozen liquor jugs. How vexingly quick daft zebras jump around the "
        "large park near the quiet zoo entrance gate"},
    {3, "Ethernet provides a framework for data transmission across local area "
        "networks using frame based protocols with MAC addressing and carrier "
        "sense multiple access with collision detection for shared medium"},
    {3, "A hash function maps data of arbitrary size to fixed size values. "
        "Cryptographic hashes like SHA-256 and BLAKE2 provide preimage "
        "resistance and collision resistance for security applications"},
    {3, "The rendering pipeline transforms three dimensional vertices through "
        "model view and projection matrices then rasterizes primitives and "
        "applies fragment shaders for final pixel color output to the display"},
    {3, "Functional programming emphasizes immutability pure functions and "
        "higher order abstractions. Languages like Haskell and Lisp employ "
        "lambda calculus as their foundational model of computation"},
    {3, "TCP uses a three way handshake with SYN and ACK packets for reliable "
        "connections. Flow control uses sliding windows while congestion "
        "control employs slow start and congestion avoidance algorithms"},
    {3, "Database normalization reduces data redundancy through first second "
        "and third normal forms. Foreign key constraints maintain referential "
        "integrity across related tables in a relational schema"},
    {3, "Compiler front ends perform lexical analysis parsing and semantic "
        "analysis. Optimization passes include dead code elimination constant "
        "folding loop unrolling and register allocation for performance"},
    {3, "A binary tree is a hierarchical data structure where each node has at "
        "most two children. Traversal orders include inorder preorder and "
        "postorder depending on when the root node is visited by the algorithm"},
    {3, "Regular expressions define search patterns using literal characters "
        "and metacharacters. Engines use NFA or DFA automata to match patterns "
        "against input strings efficiently in linear time"},
};

/* ── State ─────────────────────────────────────────────────────────────── */

typedef struct {
    char  text[MAX_TEXT];        /* concatenated passage text */
    int   text_len;              /* length of text */
    int   status[MAX_TEXT];      /* 0=pending, 1=correct, -1=wrong */
    int   cursor;                /* current typing position */
    int   duration;              /* test duration in seconds */
    int   cat_idx;               /* selected category index */
    int   started;               /* has user begun typing */
    int   finished;              /* 1=time up, -1=aborted */
    struct timespec t_start;     /* when typing started */
    double elapsed;              /* seconds elapsed */
    int   total_errors;          /* cumulative error count */
    int   key_errors[256];       /* per-character error count */
} State;

static int best_wpm[NUM_DURS] = {0};

/* ── Helpers ───────────────────────────────────────────────────────────── */

static double ts_to_sec(struct timespec *ts)
{
    return ts->tv_sec + ts->tv_nsec / 1e9;
}

static double get_elapsed(struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return ts_to_sec(&now) - ts_to_sec(start);
}

/* Fisher-Yates shuffle */
static void shuffle(int *a, int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = a[i]; a[i] = a[j]; a[j] = t;
    }
}

/* Build typing text by concatenating random passages from chosen category */
static void build_text(State *s)
{
    int idx[NUM_PASSAGES], cnt = 0;
    int filter = CAT_FILTERS[s->cat_idx];

    for (int i = 0; i < NUM_PASSAGES; i++)
        if (filter == -1 || passages[i].cat == filter)
            idx[cnt++] = i;

    shuffle(idx, cnt);

    s->text_len = 0;
    int needed = s->duration * 15;   /* ~15 chars/sec max typing speed */
    if (needed > MAX_TEXT - 100) needed = MAX_TEXT - 100;

    for (int pi = 0; s->text_len < needed && pi < cnt * 4; pi++) {
        int p = idx[pi % cnt];
        int plen = (int)strlen(passages[p].text);
        if (s->text_len + plen + 2 >= MAX_TEXT) break;
        if (s->text_len > 0)
            s->text[s->text_len++] = ' ';
        memcpy(s->text + s->text_len, passages[p].text, plen);
        s->text_len += plen;
    }
    s->text[s->text_len] = '\0';
}

/* Word-wrap text into lines. Returns number of lines. */
typedef struct { int start, len; } LineInfo;

static int wrap_text(const char *text, int text_len, int width,
                     LineInfo *lines, int max_lines)
{
    int nlines = 0, pos = 0;

    while (pos < text_len && nlines < max_lines) {
        int end = pos + width;
        if (end >= text_len) {
            /* rest of text fits on one line */
            int e = text_len;
            while (e > pos && text[e - 1] == ' ') e--;
            lines[nlines].start = pos;
            lines[nlines].len   = e - pos;
            nlines++;
            break;
        }
        /* find last space before the line end */
        int brk = end;
        while (brk > pos && text[brk] != ' ') brk--;
        if (brk == pos) brk = end;   /* no space: hard break */

        int line_end = brk;
        while (line_end > pos && text[line_end - 1] == ' ') line_end--;

        lines[nlines].start = pos;
        lines[nlines].len   = line_end - pos;
        nlines++;

        pos = brk;
        while (pos < text_len && text[pos] == ' ') pos++;
    }
    return nlines;
}

/* ── Color Initialization ──────────────────────────────────────────────── */

static void init_colors(void)
{
    start_color();
    use_default_colors();

    init_pair(CP_CORRECT,   COLOR_GREEN,  -1);
    init_pair(CP_WRONG,     COLOR_RED,    -1);
    init_pair(CP_PENDING,   COLOR_WHITE,  -1);
    init_pair(CP_CURSOR,    COLOR_BLACK,   COLOR_WHITE);
    init_pair(CP_HEADER,    COLOR_CYAN,   -1);
    init_pair(CP_ACCENT,    COLOR_YELLOW, -1);
    init_pair(CP_DIM,       COLOR_WHITE,  -1);
    init_pair(CP_BAR_FILL,  COLOR_GREEN,  -1);
    init_pair(CP_BAR_EMPTY, COLOR_WHITE,  -1);
}

/* ── Drawing Utilities ─────────────────────────────────────────────────── */

static void mvprint_centered(int y, const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    mvaddstr(y, MAX(0, (COLS - len) / 2), buf);
}

static void draw_button(int y, int x, const char *label, int sel, int cp)
{
    if (sel) attron(A_REVERSE | A_BOLD);
    if (cp)  attron(COLOR_PAIR(cp));
    mvprintw(y, x, " %s ", label);
    if (cp)  attroff(COLOR_PAIR(cp));
    if (sel) attroff(A_REVERSE | A_BOLD);
}

/* ── Menu Screen ───────────────────────────────────────────────────────── */

/* Returns selections via *out_cat and *out_dur */
static void menu_screen(int *out_cat, int *out_dur)
{
    int focus = 0;     /* 0 = category row, 1 = duration row */
    int cat_sel = 0;
    int dur_sel = 1;   /* default 30s */

    for (;;) {
        clear();

        /* Title */
        attron(A_BOLD | COLOR_PAIR(CP_HEADER));
        mvprint_centered(2, "T E R M I N A L   T Y P I N G   S P E E D   T E S T");
        attroff(A_BOLD | COLOR_PAIR(CP_HEADER));

        attron(COLOR_PAIR(CP_ACCENT));
        mvprint_centered(4, "Measure your typing speed and accuracy");
        attroff(COLOR_PAIR(CP_ACCENT));

        /* Category row */
        attron(A_BOLD);
        mvaddstr(7, 4, "Category:");
        attroff(A_BOLD);
        int cx = 16;
        for (int i = 0; i < NUM_CATS; i++) {
            draw_button(7, cx, CAT_LABELS[i],
                        focus == 0 && cat_sel == i, CP_HEADER);
            cx += (int)strlen(CAT_LABELS[i]) + 3;
        }

        /* Duration row */
        attron(A_BOLD);
        mvaddstr(9, 4, "Duration:");
        attroff(A_BOLD);
        int dx = 16;
        for (int i = 0; i < NUM_DURS; i++) {
            draw_button(9, dx, DUR_LABELS[i],
                        focus == 1 && dur_sel == i, CP_HEADER);
            dx += (int)strlen(DUR_LABELS[i]) + 3;
        }

        /* Session bests */
        attron(A_DIM | COLOR_PAIR(CP_DIM));
        mvaddstr(12, 4, "Session Best:");
        for (int i = 0; i < NUM_DURS; i++) {
            if (best_wpm[i] > 0)
                mvprintw(12, 18 + i * 14, "%s: %d WPM",
                         DUR_LABELS[i], best_wpm[i]);
            else
                mvprintw(12, 18 + i * 14, "%s: --", DUR_LABELS[i]);
        }
        attroff(A_DIM | COLOR_PAIR(CP_DIM));

        /* Start prompt */
        attron(A_BOLD | COLOR_PAIR(CP_ACCENT));
        mvprint_centered(16, ">> Press SPACE or ENTER to start <<");
        attroff(A_BOLD | COLOR_PAIR(CP_ACCENT));

        /* Controls */
        attron(A_DIM);
        mvprint_centered(LINES - 3,
            "Arrows: navigate   Tab: switch row   Space/Enter: start");
        mvprint_centered(LINES - 2, "Esc: quit");
        attroff(A_DIM);

        refresh();

        int ch = getch();
        switch (ch) {
        case KEY_LEFT:
            if (focus == 0 && cat_sel > 0) cat_sel--;
            else if (focus == 1 && dur_sel > 0) dur_sel--;
            break;
        case KEY_RIGHT:
            if (focus == 0 && cat_sel < NUM_CATS - 1) cat_sel++;
            else if (focus == 1 && dur_sel < NUM_DURS - 1) dur_sel++;
            break;
        case KEY_UP: case KEY_DOWN: case '\t':
            focus = 1 - focus;
            break;
        case ' ': case '\n': case KEY_ENTER:
            *out_cat = cat_sel;
            *out_dur = dur_sel;
            return;
        case 27:
            endwin();
            exit(0);
        }
    }
}

/* ── Typing Test ───────────────────────────────────────────────────────── */

static void typing_test(State *s)
{
    nodelay(stdscr, TRUE);
    curs_set(0);

    for (;;) {
        /* Update timer */
        if (s->started && s->finished == 0) {
            s->elapsed = get_elapsed(&s->t_start);
            if (s->elapsed >= s->duration) {
                s->elapsed = s->duration;
                s->finished = 1;
            }
        }

        /* Layout */
        int area_y = 4, area_x = 2;
        int area_w = MAX(20, COLS - 4);
        int area_h = MAX(3, LINES - 8);

        /* Word-wrap */
        LineInfo lines[MAX_LINES];
        int nlines = wrap_text(s->text, s->text_len, area_w, lines, MAX_LINES);

        /* Find cursor line for scroll */
        int cursor_line = 0;
        for (int i = 0; i < nlines; i++) {
            if (s->cursor >= lines[i].start &&
                s->cursor <  lines[i].start + lines[i].len + (i < nlines - 1 ? 1 : 0)) {
                cursor_line = i;
                break;
            }
        }
        int scroll_off = MAX(0, cursor_line - area_h + 1);

        /* ── Render ── */
        clear();

        /* Header bar */
        attron(A_BOLD | COLOR_PAIR(CP_HEADER));
        mvprintw(0, 2, "%-8s", CAT_LABELS[s->cat_idx]);
        mvprintw(0, 12, "| %s", DUR_LABELS[0]); /* placeholder for duration */
        for (int i = 0; i < NUM_DURS; i++)
            if (s->duration == DURS[i])
                mvprintw(0, 14, "%s", DUR_LABELS[i]);

        if (s->started) {
            int correct = 0;
            for (int i = 0; i < s->cursor; i++)
                if (s->status[i] == 1) correct++;
            double mins = s->elapsed / 60.0;
            int wpm = mins > 0 ? (int)(correct / 5.0 / mins) : 0;
            int acc = s->cursor > 0
                ? (int)(correct * 100.0 / s->cursor + 0.5) : 100;

            mvprintw(0, 22, "|");
            attron(COLOR_PAIR(CP_ACCENT));
            mvprintw(0, 25, "%3d WPM", wpm);
            attron(COLOR_PAIR(CP_HEADER));
            mvprintw(0, 35, "|");

            if (acc >= 95)      attron(COLOR_PAIR(CP_CORRECT));
            else if (acc >= 85) attron(COLOR_PAIR(CP_ACCENT));
            else                attron(COLOR_PAIR(CP_WRONG));
            mvprintw(0, 38, "%3d%% acc", acc);
            attron(COLOR_PAIR(CP_HEADER));

            int remain = MAX(0, s->duration - (int)s->elapsed);
            mvprintw(0, COLS - 12, "| %3ds left", remain);
        }
        attroff(A_BOLD | COLOR_PAIR(CP_HEADER));

        /* Separator */
        attron(A_DIM);
        mvhline(1, 0, ACS_HLINE, COLS);
        attroff(A_DIM);

        /* Prompt */
        if (!s->started) {
            attron(A_BOLD | COLOR_PAIR(CP_ACCENT));
            mvprint_centered(2, "Start typing to begin the test...");
            attroff(A_BOLD | COLOR_PAIR(CP_ACCENT));
        }

        /* Text area */
        for (int li = scroll_off; li < nlines && li < scroll_off + area_h; li++) {
            int sy = area_y + (li - scroll_off);
            LineInfo *ln = &lines[li];

            for (int j = 0; j < ln->len; j++) {
                int idx = ln->start + j;
                char c = s->text[idx];
                int sx = area_x + j;

                if (idx < s->cursor) {
                    if (s->status[idx] == 1) {
                        attron(COLOR_PAIR(CP_CORRECT));
                        mvaddch(sy, sx, c);
                        attroff(COLOR_PAIR(CP_CORRECT));
                    } else {
                        attron(COLOR_PAIR(CP_WRONG) | A_BOLD);
                        mvaddch(sy, sx, c == ' ' ? '_' : c);
                        attroff(COLOR_PAIR(CP_WRONG) | A_BOLD);
                    }
                } else if (idx == s->cursor && !s->finished) {
                    attron(COLOR_PAIR(CP_CURSOR) | A_BOLD);
                    mvaddch(sy, sx, c);
                    attroff(COLOR_PAIR(CP_CURSOR) | A_BOLD);
                } else {
                    attron(COLOR_PAIR(CP_PENDING) | A_DIM);
                    mvaddch(sy, sx, c);
                    attroff(COLOR_PAIR(CP_PENDING) | A_DIM);
                }
            }
        }

        /* Progress bar */
        int bar_y = LINES - 2;
        int bar_w = MAX(10, COLS - 10);
        double pct = s->started ? s->elapsed / s->duration : 0;
        if (pct > 1.0) pct = 1.0;
        int filled = (int)(bar_w * pct);

        attron(COLOR_PAIR(CP_BAR_FILL));
        for (int i = 0; i < filled; i++)
            mvaddch(bar_y, 2 + i, ACS_BLOCK);
        attron(COLOR_PAIR(CP_BAR_EMPTY) | A_DIM);
        for (int i = filled; i < bar_w; i++)
            mvaddch(bar_y, 2 + i, ACS_BLOCK);
        attroff(COLOR_PAIR(CP_BAR_EMPTY) | A_DIM);
        attron(A_BOLD);
        mvprintw(bar_y, bar_w + 4, "%3.0f%%", pct * 100);
        attroff(A_BOLD);

        refresh();

        /* ── Input ── */
        int ch = getch();
        if (ch == ERR) {
            if (s->finished == 1) { nodelay(stdscr, FALSE); return; }
            continue;
        }

        if (ch == 27) {               /* ESC -> abort */
            s->finished = -1;
            nodelay(stdscr, FALSE);
            return;
        }
        if (s->finished) continue;

        /* Backspace */
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (s->cursor > 0) {
                s->cursor--;
                s->status[s->cursor] = 0;
            }
            continue;
        }

        /* Only printable ASCII */
        if (ch < 32 || ch > 126) continue;

        /* Start timer on first keypress */
        if (!s->started) {
            s->started = 1;
            clock_gettime(CLOCK_MONOTONIC, &s->t_start);
        }

        /* Check character */
        if (s->cursor < s->text_len) {
            if (ch == s->text[s->cursor]) {
                s->status[s->cursor] = 1;
            } else {
                s->status[s->cursor] = -1;
                s->total_errors++;
                s->key_errors[(unsigned char)s->text[s->cursor]]++;
            }
            s->cursor++;
        }

        if (s->cursor >= s->text_len) s->finished = 1;
    }
}

/* ── Results Screen ────────────────────────────────────────────────────── */

/* Returns: 0 = retry, -1 = back to menu */
static int show_results(State *s, int dur_idx)
{
    int correct = 0;
    for (int i = 0; i < s->cursor; i++)
        if (s->status[i] == 1) correct++;

    double mins = s->elapsed / 60.0;
    int wpm = mins > 0 ? (int)(correct / 5.0 / mins + 0.5) : 0;
    int acc = s->cursor > 0 ? (int)(correct * 100.0 / s->cursor + 0.5) : 100;

    if (wpm > best_wpm[dur_idx]) best_wpm[dur_idx] = wpm;

    /* Rating */
    const char *rating;
    int rate_cp;
    if      (wpm >= 100) { rating = "Legendary!";    rate_cp = CP_ACCENT;   }
    else if (wpm >= 80)  { rating = "Excellent!";    rate_cp = CP_CORRECT;  }
    else if (wpm >= 60)  { rating = "Great!";        rate_cp = CP_ACCENT;   }
    else if (wpm >= 40)  { rating = "Good";          rate_cp = CP_HEADER;   }
    else if (wpm >= 20)  { rating = "Keep practicing"; rate_cp = CP_DIM;    }
    else                 { rating = "Just getting started"; rate_cp = CP_WRONG; }

    /* Find top problem keys */
    typedef struct { char ch; int n; } KeyErr;
    KeyErr top[MAX_PROBLEMS];
    memset(top, 0, sizeof(top));

    for (int i = 0; i < 256; i++) {
        if (s->key_errors[i] == 0) continue;
        for (int j = 0; j < MAX_PROBLEMS; j++) {
            if (s->key_errors[i] > top[j].n) {
                for (int k = MAX_PROBLEMS - 1; k > j; k--)
                    top[k] = top[k - 1];
                top[j].ch = (char)i;
                top[j].n  = s->key_errors[i];
                break;
            }
        }
    }

    /* Render */
    clear();

    attron(A_BOLD | COLOR_PAIR(CP_HEADER));
    mvprint_centered(2, "R E S U L T S");
    attroff(A_BOLD | COLOR_PAIR(CP_HEADER));

    /* Big WPM display */
    attron(A_BOLD | COLOR_PAIR(rate_cp));
    mvprint_centered(5, "  %d WPM  ", wpm);
    attroff(A_BOLD | COLOR_PAIR(rate_cp));
    attron(COLOR_PAIR(rate_cp));
    mvprint_centered(7, "%s", rating);
    attroff(COLOR_PAIR(rate_cp));

    /* New best? */
    if (wpm == best_wpm[dur_idx]) {
        attron(A_BOLD | COLOR_PAIR(CP_ACCENT));
        mvprint_centered(9, "* New Session Best! *");
        attroff(A_BOLD | COLOR_PAIR(CP_ACCENT));
    }

    /* Stats table */
    int y = 11;
    attron(A_BOLD); mvaddstr(y, 10, "Accuracy:   "); attroff(A_BOLD);
    attron(acc >= 95 ? COLOR_PAIR(CP_CORRECT) :
           acc >= 85 ? COLOR_PAIR(CP_ACCENT) : COLOR_PAIR(CP_WRONG));
    printw("%d%%", acc);
    attroff(COLOR_PAIR(CP_CORRECT));

    y++;
    attron(A_BOLD); mvaddstr(y, 10, "Characters: "); attroff(A_BOLD);
    printw("%d correct / %d typed", correct, s->cursor);

    y++;
    attron(A_BOLD); mvaddstr(y, 10, "Errors:     "); attroff(A_BOLD);
    printw("%d", s->total_errors);

    y++;
    attron(A_BOLD); mvaddstr(y, 10, "Time:       "); attroff(A_BOLD);
    printw("%.1f seconds", s->elapsed);

    /* Problem keys */
    if (top[0].n > 0) {
        y += 2;
        attron(A_BOLD | COLOR_PAIR(CP_ACCENT));
        mvaddstr(y, 10, "Problem Keys:");
        attroff(A_BOLD | COLOR_PAIR(CP_ACCENT));
        y++;

        int kx = 10;
        for (int i = 0; i < MAX_PROBLEMS && top[i].n > 0; i++) {
            char display = top[i].ch == ' ' ? '_' : top[i].ch;
            attron(COLOR_PAIR(CP_WRONG));
            mvprintw(y, kx, "'%c' x%d", display, top[i].n);
            attroff(COLOR_PAIR(CP_WRONG));
            kx += 14;
        }
    }

    /* WPM breakdown by category */
    y += 2;
    attron(A_DIM);
    mvprint_centered(LINES - 4,
        "SPACE: retry same test   R: new text   ESC: menu   Q: quit");
    attroff(A_DIM);

    refresh();

    for (;;) {
        int ch = getch();
        if (ch == ' ') return 0;            /* retry same text */
        if (ch == 'r' || ch == 'R') return 1; /* new text */
        if (ch == 27 || ch == 'q' || ch == 'Q') return -1;
    }
}

/* ── Main ──────────────────────────────────────────────────────────────── */

int main(void)
{
    /* ncurses init */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colors();
    srand((unsigned)time(NULL));

    /* Check minimum terminal size */
    if (COLS < 40 || LINES < 12) {
        endwin();
        fprintf(stderr, "Terminal too small (need >= 40x12). Got %dx%d.\n",
                COLS, LINES);
        return 1;
    }

    for (;;) {
        int cat, dur;
        menu_screen(&cat, &dur);

        State s;
        memset(&s, 0, sizeof(s));
        s.cat_idx  = cat;
        s.duration = DURS[dur];
        build_text(&s);

        int action = 0;
        for (;;) {
            /* Reset typing state but keep same text if retrying */
            if (action == 1) {
                /* New text requested */
                build_text(&s);
            }
            memset(s.status, 0, sizeof(int) * s.text_len);
            s.cursor       = 0;
            s.started      = 0;
            s.finished     = 0;
            s.elapsed      = 0;
            s.total_errors = 0;
            memset(s.key_errors, 0, sizeof(s.key_errors));

            typing_test(&s);

            if (s.finished == -1) break;    /* ESC -> menu */

            action = show_results(&s, dur);
            if (action < 0) break;          /* ESC/Q -> menu */
            /* 0 = retry same text, 1 = new text */
        }
    }

    /* never reached */
    endwin();
    return 0;
}
