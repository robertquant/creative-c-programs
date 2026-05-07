/*
 * typist-5994.c — Terminal Typing Speed Test
 *
 * A feature-rich typing speed test with real-time character feedback,
 * live WPM tracking with sparkline, accuracy stats, multiple passages,
 * difficulty levels, and performance grading.
 *
 * Compile:
 *   macOS: gcc -std=gnu11 -Wall -O2 -D_DARWIN_C_SOURCE -o typist typist-5994.c -lncurses -lm
 *   Linux: gcc -std=gnu11 -Wall -O2 -o typist typist-5994.c -lncurses -lm
 *
 * Or just: make typist
 *
 * Requires: ncurses, math library
 *
 * Controls:
 *   Type the displayed text — green = correct, red = wrong
 *   Backspace — delete last typed character
 *   Tab — skip to next passage
 *   Esc — return to menu
 */

#ifdef __APPLE__
#  ifndef _DARWIN_C_SOURCE
#    define _DARWIN_C_SOURCE
#  endif
#endif

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <signal.h>
#include <locale.h>

/* ── Constants ─────────────────────────────────────────────── */

#define NUM_PASSAGES  14
#define SPARK_W       50
#define PASSAGE_W     64

enum {
    CP_CORRECT = 1, CP_WRONG, CP_PENDING, CP_TITLE, CP_ACCENT,
    CP_DIM, CP_BAR_BG, CP_BAR_FG, CP_HEADER,
    CP_GRADE_S, CP_GRADE_A, CP_GRADE_B, CP_GRADE_C, CP_GRADE_F,
    CP_CURSOR
};

/* ── Passage Data ──────────────────────────────────────────── */

typedef struct {
    const char *label;
    const char *text;
    int difficulty; /* 1 = easy, 2 = medium, 3 = hard */
} Passage;

static const Passage passages[NUM_PASSAGES] = {
    /* ── Easy ── */
    {"The Quick Brown Fox",
     "The quick brown fox jumps over the lazy dog. "
     "Pack my box with five dozen liquor jugs. "
     "How vexingly quick daft zebras jump. "
     "The five boxing wizards jump quickly.",
     1},

    {"Simple Morning",
     "Good morning! Today is a beautiful day. "
     "The sun is shining and the birds are singing. "
     "Let us make the most of this wonderful day. "
     "Smile and be happy.",
     1},

    {"Nature Walk",
     "A gentle breeze rustled through the tall oak trees. "
     "The river flowed quietly over smooth stones. "
     "Butterflies danced among the wildflowers. "
     "It was a perfect afternoon for a walk in the park.",
     1},

    /* ── Medium ── */
    {"Programming Wisdom",
     "\"First, solve the problem. Then, write the code.\" "
     "This timeless advice reminds us that understanding comes "
     "before implementation. Good programmers think deeply about "
     "the problem domain before writing a single line of code.",
     2},

    {"The C Language",
     "The C programming language, created by Dennis Ritchie at "
     "Bell Labs in 1972, remains one of the most influential "
     "languages in computing history. Its combination of low-level "
     "access and high-level constructs shaped modern software.",
     2},

    {"Unix Philosophy",
     "Write programs that do one thing and do it well. Write "
     "programs to work together. Write programs to handle text "
     "streams, because that is a universal interface. "
     "-- Doug McIlroy, inventor of Unix pipes.",
     2},

    {"Digital Age",
     "In the age of digital communication, the ability to type "
     "quickly and accurately has become an essential skill. From "
     "writing emails to coding software, our fingers translate "
     "thoughts into text at remarkable speeds every single day.",
     2},

    {"Algorithms",
     "An algorithm is a finite sequence of well-defined instructions "
     "to solve a class of problems or perform a computation. "
     "Good algorithms are measured by their time and space complexity, "
     "often expressed using Big-O notation.",
     2},

    /* ── Hard ── */
    {"Code Snippet",
     "for (int i = 0; i < n; i++) {\n"
     "    if (arr[i] > max_val) {\n"
     "        max_val = arr[i];\n"
     "        max_idx = i;\n"
     "    }\n"
     "    total += arr[i] * 2;\n"
     "}\n"
     "printf(\"max=%d at %d, sum=%d\\n\", max_val, max_idx, total);",
     3},

    {"Network Protocol",
     "The TCP/IP stack operates across four layers: the link layer "
     "(Ethernet/WiFi), internet layer (IPv4/IPv6, ICMP), transport "
     "layer (TCP/UDP, ports 0-65535), and application layer (HTTP/1.1, "
     "TLS 1.3, DNS). Default MTU is 1500 bytes on Ethernet.",
     3},

    {"Order Form",
     "Order #2024-0815: 3x USB-C cables ($12.99/ea) + 2x HDMI 2.1 "
     "adapters ($24.50/ea) = $88.97 total. Ship to: 1234 Main St., "
     "Apt #5B, San Francisco, CA 94102-1847. ETA: 3-5 biz days.",
     3},

    {"Regex Pattern",
     "Pattern: ^(?<host>[\\w.]+):(?<port>\\d{1,5})$\n"
     "Matches: \"localhost:8080\" or \"192.168.1.1:443\"\n"
     "Range: port must be 0-65535; validate with (?!^0+$)(?!^6553[6-9]"
     "$)(?!^655[4-9]\\d$)(?!^65[6-9]\\d\\d$)(?!^6[6-9]\\d{3}$)"
     "(?!^[7-9]\\d{4}$)(?!^\\d{6,}$)\\d{1,5}$",
     3},

    {"Pangram Mix",
     "Sphinx of black quartz, judge my vow! "
     "Cwm fjord bank glyphs vext quiz. "
     "Jived fox nymph grabs quick waltz. "
     "Brawny gods flocked up to quiz and vex him. "
     "Pack my box with five dozen liquor jugs!",
     2},

    {"Kernel Dev",
     "The Linux kernel uses a monolithic architecture with loadable "
     "modules (LKM). Key subsystems include: scheduler (CFS/EEVDF), "
     "memory management (SLUB allocator, page tables), VFS, network "
     "stack (sk_buff), and device driver framework. sysfs/procfs "
     "expose kernel state to userspace.",
     3},
};

/* ── Session State ──────────────────────────────────────────── */

typedef struct {
    int passage_idx;
    int pos;
    int *char_status;  /* 0=pending, 1=correct, -1=wrong */
    int text_len;
    int total_keystrokes;
    int correct_keystrokes;
    double start_time;
    int started;
    int finished;
    double wpm_hist[SPARK_W];
    int wpm_n;
    double last_sample_time;
    double final_wpm;
    double final_acc;
    double elapsed;
} Session;

static volatile sig_atomic_t g_running = 1;

/* ── Utility ───────────────────────────────────────────────── */

static double now_sec(void)
{
    struct timespec ts;
#ifdef __APPLE__
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#endif
    return ts.tv_sec + ts.tv_nsec / 1.0e9;
}

static void on_sigint(int sig) { (void)sig; g_running = 0; }

static int max_int(int a, int b) { return a > b ? a : b; }

/* ── Colors ────────────────────────────────────────────────── */

static void init_colors(void)
{
    start_color();
    use_default_colors();
    init_pair(CP_CORRECT, COLOR_GREEN,  -1);
    init_pair(CP_WRONG,   COLOR_WHITE,  COLOR_RED);
    init_pair(CP_PENDING, 8,            -1);        /* dark gray */
    init_pair(CP_TITLE,   COLOR_CYAN,   -1);
    init_pair(CP_ACCENT,  COLOR_YELLOW, -1);
    init_pair(CP_DIM,     8,            -1);
    init_pair(CP_HEADER,  COLOR_YELLOW, -1);
    init_pair(CP_BAR_BG,  8,            -1);
    init_pair(CP_BAR_FG,  COLOR_GREEN,  -1);
    init_pair(CP_CURSOR,  COLOR_BLACK,  COLOR_WHITE);
    init_pair(CP_GRADE_S, COLOR_MAGENTA, -1);
    init_pair(CP_GRADE_A, COLOR_GREEN,   -1);
    init_pair(CP_GRADE_B, COLOR_CYAN,    -1);
    init_pair(CP_GRADE_C, COLOR_YELLOW,  -1);
    init_pair(CP_GRADE_F, COLOR_RED,     -1);
}

/* ── Draw helpers ──────────────────────────────────────────── */

static void draw_box(int y, int x, int h, int w)
{
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + w - 1, ACS_URCORNER);
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    mvaddch(y + h - 1, x + w - 1, ACS_LRCORNER);
    mvhline(y, x + 1, ACS_HLINE, w - 2);
    mvhline(y + h - 1, x + 1, ACS_HLINE, w - 2);
    for (int i = 1; i < h - 1; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
}

static void center_str(int row, const char *s, int attr)
{
    int len = (int)strlen(s);
    int col = max_int(0, (COLS - len) / 2);
    if (attr) attron(attr);
    mvaddstr(row, col, s);
    if (attr) attroff(attr);
}

/* ── Title Screen ──────────────────────────────────────────── */

static void show_title(void)
{
    clear();
    nodelay(stdscr, FALSE);

    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    center_str(3, " _____                    _        ", 0);
    center_str(4, "|_   _|__ _ __ _ __ __ _ | |_ ___  ", 0);
    center_str(5, "  | |/ _ \\ '__| '__/ _` || __/ _ \\ ", 0);
    center_str(6, "  | |  __/ |  | | | (_| || || (_) |", 0);
    center_str(7, "  |_|\\___|_|  |_|  \\__,_| \\__\\___/ ", 0);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    center_str(9, "Terminal Typing Speed Test", COLOR_PAIR(CP_ACCENT) | A_BOLD);

    attron(COLOR_PAIR(CP_DIM));
    center_str(11, "Test your typing speed and accuracy!", 0);
    attroff(COLOR_PAIR(CP_DIM));

    attron(A_BOLD);
    center_str(14, "Press ENTER to start", 0);
    attroff(A_BOLD);
    center_str(16, "Select passage category:", COLOR_PAIR(CP_HEADER));
    center_str(18, "[1] Easy    [2] Medium    [3] Hard    [R] Random", 0);
    center_str(20, "[Q] Quit", COLOR_PAIR(CP_DIM));

    refresh();
}

/* ── Menu: pick passage ─────────────────────────────────────── */

static int pick_passage(int difficulty)
{
    /* Collect matching indices */
    int candidates[NUM_PASSAGES];
    int n = 0;
    for (int i = 0; i < NUM_PASSAGES; i++) {
        if (difficulty == 0 || passages[i].difficulty == difficulty)
            candidates[n++] = i;
    }
    if (n == 0) return 0;
    return candidates[rand() % n];
}

/* ── WPM Calculation ───────────────────────────────────────── */

static double calc_wpm(int correct, double elapsed_sec)
{
    if (elapsed_sec < 0.5) return 0.0;
    return (correct / 5.0) / (elapsed_sec / 60.0);
}

static char grade_char(double wpm, double acc)
{
    if (wpm >= 120 && acc >= 98) return 'S';
    if (wpm >= 80  && acc >= 95) return 'A';
    if (wpm >= 60  && acc >= 90) return 'B';
    if (wpm >= 40  && acc >= 85) return 'C';
    if (wpm >= 20  && acc >= 75) return 'D';
    return 'F';
}

static int grade_color(char g)
{
    switch (g) {
    case 'S': return CP_GRADE_S;
    case 'A': return CP_GRADE_A;
    case 'B': return CP_GRADE_B;
    case 'C': return CP_GRADE_C;
    default:  return CP_GRADE_F;
    }
}

/* ── Draw Passage Text ─────────────────────────────────────── */

/* Word-wrap the text into lines, render with color coding */
static void draw_passage(const Session *s, int top_y)
{
    const char *txt = passages[s->passage_idx].text;
    int len = s->text_len;
    int cx = 2, cy = top_y;      /* start inside box padding */
    int margin_left = (COLS - PASSAGE_W) / 2;
    if (margin_left < 2) margin_left = 2;
    int wrap_w = COLS - margin_left - 4;
    if (wrap_w < 20) wrap_w = 20;
    if (wrap_w > PASSAGE_W) wrap_w = PASSAGE_W;

    cx = margin_left + 1;
    cy = top_y;

    for (int i = 0; i < len; i++) {
        if (txt[i] == '\n') {
            cx = margin_left + 1;
            cy++;
            continue;
        }
        if (cx - margin_left > wrap_w) {
            cx = margin_left + 1;
            cy++;
        }

        int status = (i < s->pos) ? s->char_status[i] : 0;
        if (i < s->pos) {
            if (status == 1) attron(COLOR_PAIR(CP_CORRECT));
            else             attron(COLOR_PAIR(CP_WRONG));
        } else if (i == s->pos && !s->finished) {
            attron(COLOR_PAIR(CP_CURSOR) | A_BOLD);
        } else {
            attron(COLOR_PAIR(CP_PENDING));
        }

        char ch = (txt[i] < 32 || txt[i] > 126) ? ' ' : txt[i];
        mvaddch(cy, cx, ch);

        /* Reset attributes */
        if (i < s->pos) {
            if (status == 1) attroff(COLOR_PAIR(CP_CORRECT));
            else             attroff(COLOR_PAIR(CP_WRONG));
        } else if (i == s->pos && !s->finished) {
            attroff(COLOR_PAIR(CP_CURSOR) | A_BOLD);
        } else {
            attroff(COLOR_PAIR(CP_PENDING));
        }

        cx++;
    }
}

/* ── Draw Stats Panel ──────────────────────────────────────── */

static void draw_stats(const Session *s, int y)
{
    const char *diff_str[] = {"", "Easy", "Medium", "Hard"};
    int diff = passages[s->passage_idx].difficulty;

    attron(COLOR_PAIR(CP_HEADER));
    mvprintw(y, 2, " Passage: \"%s\" [%s]",
             passages[s->passage_idx].label, diff_str[diff]);
    attroff(COLOR_PAIR(CP_HEADER));

    if (!s->started) {
        attron(COLOR_PAIR(CP_DIM));
        mvprintw(y + 1, 2, " Start typing... (the timer begins on first keypress)");
        attroff(COLOR_PAIR(CP_DIM));
        return;
    }

    double elapsed = s->elapsed;
    double wpm = calc_wpm(s->correct_keystrokes, elapsed);
    double acc = s->total_keystrokes > 0
        ? (s->correct_keystrokes * 100.0 / s->total_keystrokes) : 100.0;

    int mins = (int)(elapsed) / 60;
    int secs = (int)(elapsed) % 60;
    int pct = s->text_len > 0 ? (s->pos * 100 / s->text_len) : 0;

    /* WPM */
    attron(A_BOLD);
    mvprintw(y + 1, 2, " WPM:");
    attroff(A_BOLD);
    attron(COLOR_PAIR(CP_ACCENT));
    printw(" %5.1f", wpm);
    attroff(COLOR_PAIR(CP_ACCENT));

    /* Accuracy */
    attron(A_BOLD);
    printw("   Accuracy:");
    attroff(A_BOLD);
    if (acc >= 95)      attron(COLOR_PAIR(CP_CORRECT));
    else if (acc >= 85) attron(COLOR_PAIR(CP_ACCENT));
    else                attron(COLOR_PAIR(CP_WRONG));
    printw(" %5.1f%%", acc);
    attroff(COLOR_PAIR(CP_CORRECT));
    attroff(COLOR_PAIR(CP_ACCENT));
    attroff(COLOR_PAIR(CP_WRONG));

    /* Time */
    attron(A_BOLD);
    printw("   Time:");
    attroff(A_BOLD);
    printw(" %d:%02d", mins, secs);

    /* Progress */
    attron(A_BOLD);
    printw("   Progress:");
    attroff(A_BOLD);
    printw(" %d%%", pct);

    /* Progress bar */
    int bar_y = y + 2;
    int bar_w = COLS - 6;
    if (bar_w > 60) bar_w = 60;
    int filled = bar_w * pct / 100;

    mvaddch(bar_y, 2, '[');
    for (int i = 0; i < bar_w; i++) {
        if (i < filled) {
            attron(COLOR_PAIR(CP_BAR_FG));
            addch(ACS_CKBOARD);
            attroff(COLOR_PAIR(CP_BAR_FG));
        } else {
            attron(COLOR_PAIR(CP_BAR_BG));
            addch('.');
            attroff(COLOR_PAIR(CP_BAR_BG));
        }
    }
    addch(']');

    /* Sparkline */
    if (s->wpm_n > 1) {
        int sp_y = y + 3;
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(sp_y, 2, " WPM History: ");
        attroff(COLOR_PAIR(CP_DIM));
        const char *blocks = " .:;+=*#%@";
        int nblocks = (int)strlen(blocks);
        double mx = 0;
        for (int i = 0; i < s->wpm_n; i++)
            if (s->wpm_hist[i] > mx) mx = s->wpm_hist[i];
        if (mx < 1) mx = 1;

        int sp_x = 16;
        int avail = COLS - sp_x - 4;
        if (avail < 10) avail = 10;
        int start = s->wpm_n > avail ? s->wpm_n - avail : 0;
        for (int i = start; i < s->wpm_n; i++) {
            int idx = (int)((s->wpm_hist[i] / mx) * (nblocks - 1));
            if (idx >= nblocks) idx = nblocks - 1;
            if (idx < 0) idx = 0;
            int ci = idx * 100 / (nblocks - 1);
            if (ci > 70)      attron(COLOR_PAIR(CP_CORRECT));
            else if (ci > 40) attron(COLOR_PAIR(CP_ACCENT));
            else              attron(COLOR_PAIR(CP_DIM));
            addch(blocks[idx]);
            attroff(COLOR_PAIR(CP_CORRECT));
            attroff(COLOR_PAIR(CP_ACCENT));
            attroff(COLOR_PAIR(CP_DIM));
        }
    }
}

/* ── Results Screen ─────────────────────────────────────────── */

static char show_results(Session *s)
{
    double wpm = s->final_wpm;
    double acc = s->final_acc;
    char grade = grade_char(wpm, acc);
    int gc = grade_color(grade);
    int mins = (int)(s->elapsed) / 60;
    int secs = (int)(s->elapsed) % 60;

    nodelay(stdscr, FALSE);
    clear();

    int bh = 20, bw = 52;
    int by = (LINES - bh) / 2;
    int bx = (COLS - bw) / 2;
    if (by < 0) by = 0;
    if (bx < 0) bx = 0;

    draw_box(by, bx, bh, bw);

    attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
    center_str(by + 1, "RESULTS", 0);
    attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

    int cy = by + 3;
    int cx = bx + 4;

    mvprintw(cy, cx,     "Passage:   %s", passages[s->passage_idx].label);
    cy++;

    attron(A_BOLD);
    mvprintw(cy, cx, "WPM:");
    attroff(A_BOLD);
    attron(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    printw("       %.1f", wpm);
    attroff(COLOR_PAIR(CP_ACCENT) | A_BOLD);
    cy++;

    attron(A_BOLD);
    mvprintw(cy, cx, "Accuracy:");
    attroff(A_BOLD);
    if (acc >= 95)      attron(COLOR_PAIR(CP_CORRECT));
    else if (acc >= 85) attron(COLOR_PAIR(CP_ACCENT));
    else                attron(COLOR_PAIR(CP_WRONG));
    printw("   %.1f%%", acc);
    attroff(COLOR_PAIR(CP_CORRECT));
    attroff(COLOR_PAIR(CP_ACCENT));
    attroff(COLOR_PAIR(CP_WRONG));
    cy++;

    mvprintw(cy, cx, "Time:      %d:%02d", mins, secs);
    cy++;

    mvprintw(cy, cx, "Chars:     %d correct, %d errors, %d total",
             s->correct_keystrokes,
             s->total_keystrokes - s->correct_keystrokes,
             s->text_len);
    cy += 2;

    /* Grade display — big and centered */
    int gx = bx + bw / 2;
    attron(COLOR_PAIR(gc) | A_BOLD);
    mvaddch(cy, gx - 5,    ' ');
    mvaddch(cy, gx - 4,    ACS_ULCORNER); mvaddch(cy, gx - 3, ACS_HLINE);
    mvaddch(cy, gx - 2,    ACS_HLINE);    mvaddch(cy, gx - 1, ACS_HLINE);
    mvaddch(cy, gx,        ACS_URCORNER);
    cy++;

    mvaddch(cy, gx - 5, ACS_VLINE); mvaddstr(cy, gx - 4, "     ");
    attron(A_REVERSE);
    mvaddch(cy, gx - 3, ' ');
    mvaddch(cy, gx - 2, grade);
    mvaddch(cy, gx - 1, ' ');
    attroff(A_REVERSE);
    mvaddch(cy, gx, ACS_VLINE);
    cy++;

    mvaddch(cy, gx - 5,    ' ');
    mvaddch(cy, gx - 4,    ACS_LLCORNER); mvaddch(cy, gx - 3, ACS_HLINE);
    mvaddch(cy, gx - 2,    ACS_HLINE);    mvaddch(cy, gx - 1, ACS_HLINE);
    mvaddch(cy, gx,        ACS_LRCORNER);
    attroff(COLOR_PAIR(gc) | A_BOLD);
    cy += 2;

    /* Sparkline of WPM over time */
    if (s->wpm_n > 1) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(cy, bx + 4, "WPM: ");
        attroff(COLOR_PAIR(CP_DIM));
        const char *blocks = " .:;+=*#%@";
        int nblocks = (int)strlen(blocks);
        double mx = 0;
        for (int i = 0; i < s->wpm_n; i++)
            if (s->wpm_hist[i] > mx) mx = s->wpm_hist[i];
        if (mx < 1) mx = 1;
        int avail = bw - 12;
        int start = s->wpm_n > avail ? s->wpm_n - avail : 0;
        for (int i = start; i < s->wpm_n; i++) {
            int idx = (int)((s->wpm_hist[i] / mx) * (nblocks - 1));
            if (idx >= nblocks) idx = nblocks - 1;
            if (idx < 0) idx = 0;
            addch(blocks[idx]);
        }
        cy += 2;
    }

    attron(COLOR_PAIR(CP_DIM));
    center_str(by + bh - 2,
               "[R] Retry   [N] Next   [Q] Quit", 0);
    attroff(COLOR_PAIR(CP_DIM));

    refresh();

    while (g_running) {
        int ch = getch();
        if (ch == 'r' || ch == 'R') return 'r';
        if (ch == 'n' || ch == 'N') return 'n';
        if (ch == 'q' || ch == 'Q' || ch == 27) return 'q';
    }
    return 'q';
}

/* ── Main Typing Loop ──────────────────────────────────────── */

static void run_typing(int passage_idx)
{
    Session s;
    memset(&s, 0, sizeof(s));
    s.passage_idx = passage_idx;
    s.text_len = (int)strlen(passages[passage_idx].text);
    s.char_status = calloc(s.text_len, sizeof(int));
    if (!s.char_status) return;
    s.started = 0;
    s.finished = 0;
    s.wpm_n = 0;

    int result = 0; /* 0 = running */

    while (g_running && !result) {
        /* Calculate elapsed time */
        if (s.started && !s.finished)
            s.elapsed = now_sec() - s.start_time;

        /* ── Draw ── */
        clear();
        int box_h = LINES - 2;
        int box_w = COLS;
        if (box_h < 10) box_h = 10;
        draw_box(0, 0, box_h, box_w);

        /* Header */
        attron(COLOR_PAIR(CP_TITLE) | A_BOLD);
        center_str(1, "TYPIST  --  Terminal Typing Speed Test", 0);
        attroff(COLOR_PAIR(CP_TITLE) | A_BOLD);

        /* Passage text area */
        int text_top = 3;
        draw_passage(&s, text_top);

        /* Stats at bottom */
        int stats_y = LINES - 7;
        if (stats_y < text_top + 3) stats_y = text_top + 3;
        mvhline(stats_y - 1, 1, ACS_HLINE, COLS - 2);
        draw_stats(&s, stats_y);

        /* Help line */
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(LINES - 1, 2, " [TAB] skip  [ESC] menu  [BACKSPACE] delete");
        attroff(COLOR_PAIR(CP_DIM));

        refresh();

        /* ── Input ── */
        nodelay(stdscr, TRUE);
        halfdelay(2);  /* 20ms timeout for animation updates */

        int ch = getch();

        if (ch == ERR) continue;  /* timeout — just refresh */

        /* ESC */
        if (ch == 27) { result = 1; break; }

        /* Tab — skip */
        if (ch == '\t') {
            s.final_wpm = calc_wpm(s.correct_keystrokes, s.elapsed);
            s.final_acc = s.total_keystrokes > 0
                ? (s.correct_keystrokes * 100.0 / s.total_keystrokes) : 100.0;
            free(s.char_status);
            return;
        }

        /* Backspace */
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (s.pos > 0) {
                s.pos--;
                s.char_status[s.pos] = 0;
            }
            continue;
        }

        /* Regular character */
        if (ch >= 32 && ch <= 126) {
            if (s.pos < s.text_len) {
                /* Start timer on first keypress */
                if (!s.started) {
                    s.started = 1;
                    s.start_time = now_sec();
                    s.last_sample_time = s.start_time;
                }

                char expected = passages[s.passage_idx].text[s.pos];
                if (expected == '\n') expected = '\n';  /* skip newlines */

                if (expected == ch) {
                    s.char_status[s.pos] = 1;
                    s.correct_keystrokes++;
                } else {
                    s.char_status[s.pos] = -1;
                }
                s.total_keystrokes++;
                s.pos++;

                /* Update WPM history every 0.5s */
                double t = now_sec();
                if (s.started && t - s.last_sample_time >= 0.5) {
                    double w = calc_wpm(s.correct_keystrokes,
                                        t - s.start_time);
                    if (s.wpm_n < SPARK_W) {
                        s.wpm_hist[s.wpm_n++] = w;
                    } else {
                        memmove(s.wpm_hist, s.wpm_hist + 1,
                                (SPARK_W - 1) * sizeof(double));
                        s.wpm_hist[SPARK_W - 1] = w;
                    }
                    s.last_sample_time = t;
                }

                /* Check completion */
                if (s.pos >= s.text_len) {
                    s.finished = 1;
                    s.elapsed = now_sec() - s.start_time;
                    s.final_wpm = calc_wpm(s.correct_keystrokes, s.elapsed);
                    s.final_acc = s.total_keystrokes > 0
                        ? (s.correct_keystrokes * 100.0 / s.total_keystrokes)
                        : 100.0;

                    char action = show_results(&s);
                    free(s.char_status);
                    if (action == 'r') {
                        run_typing(passage_idx);
                        return;
                    } else if (action == 'n') {
                        int next = pick_passage(0);
                        run_typing(next);
                        return;
                    }
                    return;
                }
            }
        }

        /* Skip over newlines in source text */
        while (s.pos < s.text_len &&
               passages[s.passage_idx].text[s.pos] == '\n') {
            s.char_status[s.pos] = 1;
            s.correct_keystrokes++;
            s.total_keystrokes++;
            s.pos++;
        }
    }

    free(s.char_status);
}

/* ── Main ──────────────────────────────────────────────────── */

int main(void)
{
    srand((unsigned)time(NULL));
    setlocale(LC_ALL, "");

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colors();

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    while (g_running) {
        show_title();
        int ch = getch();

        if (ch == 'q' || ch == 'Q') break;
        if (ch == 27) break; /* ESC */

        int diff = 0;
        if (ch == '1') diff = 1;
        else if (ch == '2') diff = 2;
        else if (ch == '3') diff = 3;
        else if (ch == 'r' || ch == 'R' || ch == '\n') diff = 0;

        int idx = pick_passage(diff);
        run_typing(idx);
    }

    endwin();
    return 0;
}
