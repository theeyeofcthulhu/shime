/*
shime: small terminal clock
Copyright (C) 2021 eyeofcthulhu

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>
#include <unistd.h>

#include <curses.h>

// Ascii table keys
#define KEY_q 113
#define KEY_h 104
#define KEY_j 106
#define KEY_k 107
#define KEY_l 108
#define KEY_esc 27

#define START_Y 3
#define START_X 5

#define NANO_INTERVAL 25000000

typedef struct {
    char* fmt;
    char* last_next;
} DateTimeFormat;

const DateTimeFormat de = {
    .fmt       = "%d.%m.%Y %H:%M:%S",
    .last_next = "%d %m %Y %H %M %S",
};

const DateTimeFormat us = {
    .fmt       = "%m/%d/%Y %H:%M:%S",
    .last_next = "%m %d %Y %H %M %S",
};

typedef struct {
    int y;
    int x;
    int height;
    int width;
} Dimensions;

DateTimeFormat strtimeformat(char* str);

void finish(int sig);

void get_last_next_hours_mins_or_seconds(int n, int max, int *last, int *next);
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next);

int days_in_month(int mon, int year);

DateTimeFormat strtimeformat(char* str)
{
    if(strcmp(str, "de") == 0) {
        return de;
    } else if(strcmp(str, "us") == 0) {
        return us;
    } else {
        fprintf(stderr, "Invalid format: '%s'\n", str);
        exit(1);
    }
}

// Cleanly exit ncurses
void finish(int sig)
{
    endwin();
    if (sig == SIGSEGV) {
        fprintf(stderr, "Segfault\n");
        exit(1);
    }
    exit(0);
}

void get_last_next_hours_mins_or_seconds(int n, int max, int *last, int *next)
{
    if (n + 1 >= max) {
        *next = 0;
    } else {
        *next = n + 1;
    }
    if (n - 1 <= -1) {
        *last = max - 1;
    } else {
        *last = n - 1;
    }
}

// Calculate the last and the next time unit for every unit in cur
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next)
{
    int year = cur.tm_year + 1900;
    int mon = cur.tm_mon + 1;

    // Days
    if (cur.tm_mday + 1 > days_in_month(mon, year)) {
        next->tm_mday = 1;
    } else {
        next->tm_mday = cur.tm_mday + 1;
    }
    if (cur.tm_mday - 1 <= 0) {
        if (mon - 1 <= 0) {
            // Last day is in December of last year
            last->tm_mday = days_in_month(12, year - 1);
        } else {
            last->tm_mday = days_in_month(mon - 1, year);
        }
    } else {
        last->tm_mday = cur.tm_mday - 1;
    }

    // Months
    if (mon + 1 == 13) {
        next->tm_mon = 0;
    } else {
        next->tm_mon = cur.tm_mon + 1;
    }
    if (mon - 1 == 0) {
        last->tm_mon = 11;
    } else {
        last->tm_mon = cur.tm_mon - 1;
    }

    // Years
    next->tm_year = cur.tm_year + 1;
    last->tm_year = cur.tm_year - 1;

    get_last_next_hours_mins_or_seconds(cur.tm_hour, 24, &last->tm_hour,
                                        &next->tm_hour); // Hours
    get_last_next_hours_mins_or_seconds(cur.tm_min, 60, &last->tm_min,
                                        &next->tm_min); // Minutes
    get_last_next_hours_mins_or_seconds(cur.tm_sec, 60, &last->tm_sec,
                                        &next->tm_sec); // Seconds
}

// Return the days a month has
int days_in_month(int mon, int year)
{
    assert(mon >= 1 && mon <= 12);
    switch (mon) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        return 31;
    case 4:
    case 6:
    case 9:
    case 11:
        return 30;
    case 2:
        // Leap year: https://en.wikipedia.org/wiki/Leap_year#Algorithm
        if (year % 4 == 0 && !(year % 100 == 0 && year % 400 != 0))
            return 29;
        else
            return 28;
    default:
        assert(false);
        return 0;
    }
}

int main(int argc, char **argv)
{
    DateTimeFormat format = de;

    // TODO: add other data/time formats
    // Argparse with getopt()
    int arg;
    while ((arg = getopt(argc, argv, "hf:")) != -1) {
        switch (arg) {
        case 'h':
            // Help page
            printf("shime Copyright (C) 2021 eyeofcthulhu\n\n"
                   "options:\n"
                   "-h: display help\n"
                   "-f [de us]: change the time format\n"
                   "-u: display usage\n\n"
                   "controls\n"
                   "vim keys - hjkl or arrow keys: move around\n"
                   "q or esc: exit\n");
            exit(0);
            break;
        case 'f':
            format = strtimeformat(optarg);
            break;
        case '?':
        default:
            return 1;
            break;
        }
    }

    // Ncurses init logic

    // On interrupt (Ctrl+c) exit
    signal(SIGINT, finish);
    signal(SIGKILL, finish);
    signal(SIGSEGV, finish);

    // Init
    initscr();

    // Return key doesn't become newline
    nonl();

    // Disable curosr
    curs_set(0);

    // Allows Ctrl+c to quit the program
    cbreak();

    // Don't echo the the getch() chars onto the screen
    noecho();

    // Getch() doesn't wait for input and just returns ERR if no key is pressed
    nodelay(stdscr, true);

    // Enable keypad (for arrow keys)
    keypad(stdscr, true);

    // Color support
    if (!has_colors()) {
        fprintf(stderr, "Your terminal does not support color\n");
        finish(SIGKILL);
    }

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);

    // Grey color
    init_color(COLOR_BLUE, 200, 200, 200);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    // Init dimensions
    Dimensions dimensions;
    getmaxyx(stdscr, dimensions.height, dimensions.width);

    dimensions.y = START_Y;
    dimensions.x = START_X;

    // Initialize and get the localtime
    time_t cur_time = time(NULL);
    struct tm *local_time = localtime(&cur_time);

    // Timer on with to update the clock
    int timer = 0;
    const int timer_re = 4;

    // How much we want to sleep every tick
    const struct timespec request = {0, NANO_INTERVAL};

    char buf[128];

    bool running = true;

    // The main loop: update, draw, sleep
    while (running) {
        switch (wgetch(stdscr)) {
        case KEY_esc:
        case KEY_q:
            running = false;
            break;
        /* If we move here we also need to redraw the screen, hence the call to
         * erase() which isn't called normally */
        case KEY_LEFT:
        case KEY_h:
            erase();
            if ((dimensions.x - 1) > 0)
                dimensions.x -= 1;
            break;
        case KEY_DOWN:
        case KEY_j:
            erase();
            if ((dimensions.y + 1) < dimensions.height - 1)
                dimensions.y += 1;
            break;
        case KEY_UP:
        case KEY_k:
            erase();
            if ((dimensions.y - 1) > 0)
                dimensions.y -= 1;
            break;
        case KEY_RIGHT:
        case KEY_l:
            erase();
            if ((dimensions.x + 1) < dimensions.width - 19)
                dimensions.x += 1;
            break;
        default:
            break;
        }

        // Update clock every four ticks (1 second)
        timer++;
        if (timer == timer_re) {
            cur_time = time(NULL);
            *local_time = *localtime(&cur_time);

            // Color for the clock, defined in init()
            attron(COLOR_PAIR(1));
            move(dimensions.y, dimensions.x);

            strftime(buf, sizeof(buf), format.fmt, local_time);

            // Draw the date and time to the screen
            addstr(buf);

            // Draw the previous and next times
            struct tm last, next;
            get_last_next_time(*local_time, &last, &next);

            attron(COLOR_PAIR(2));

            strftime(buf, sizeof(buf), format.last_next, &last);
            mvaddstr(dimensions.y - 1, dimensions.x, buf);

            strftime(buf, sizeof(buf), format.last_next, &next);
            mvaddstr(dimensions.y + 1, dimensions.x, buf);

            refresh();

            timer = 0;

            thrd_sleep(&request, NULL);
        }
    }

    endwin();

    return 0;
}
