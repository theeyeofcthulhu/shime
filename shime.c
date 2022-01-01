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
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <curses.h>

// ascii table keys
#define KEY_q 113
#define KEY_h 104
#define KEY_j 106
#define KEY_k 107
#define KEY_l 108
#define KEY_esc 27

#define START_Y 3
#define START_X 5

#define U_INTERVALL 25000

void finish(int sig);

void last_next_hours_mins_or_seconds(int y, int x, int unit, int max);
void last_next_day(int y, int x, int day, int mon, int year);
void last_next_mon(int y, int x, int mon);
void last_next_year(int y, int x, int year);
void addstr_last_next(int y, int x, int next, int last, int len);

int days_in_month(int mon, int year);

typedef struct {
    int y;
    int x;
    int height;
    int width;
} dimensions;

int main(int argc, char **argv)
{
    // TODO: add other data/time formats
    // argparse with getopt()
    int arg;
    while ((arg = getopt(argc, argv, "hu")) != -1) {
        switch (arg) {
        case 'h':
            // help page
            printf("shime Copyright (C) 2021 eyeofcthulhu\n\n"
                   "options:\n"
                   "h: display help\n"
                   "u: display usage\n\n"
                   "controls\n"
                   "vim keys - hjkl or arrow keys: move around\n"
                   "q or esc: exit\n");
            exit(0);
            break;
        default:
        case 'u':
            printf("Usage: %s [-hu]\n", argv[0]);
            exit(0);
            break;
        }
    }

    // ncurses init logic
    // on interrupt (Ctrl+c) exit
    signal(SIGINT, finish);
    signal(SIGSEGV, finish);

    // init
    initscr();

    // return key doesn't become newline
    nonl();

    // disable curosr
    curs_set(0);

    // allows Ctrl+c to quit the program
    cbreak();

    // don't echo the the getch() chars onto the screen
    noecho();

    // getch() doesn't wait for input and just returns ERR if no key is pressed
    nodelay(stdscr, true);

    // enable keypad (for arrow keys)
    keypad(stdscr, true);

    // color support
    if (!has_colors()) {
        fprintf(stderr, "Your terminal does not support color\n");
        finish(0);
    }

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);

    // grey color
    init_color(COLOR_BLUE, 200, 200, 200);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    // Init dimensions
    dimensions dimensions;
    getmaxyx(stdscr, dimensions.height, dimensions.width);

    dimensions.y = START_Y;
    dimensions.x = START_X;

    // initialize and get the localtime
    time_t cur_time = time(NULL);
    struct tm *local_time = localtime(&cur_time);

    // timer on with to update the clock
    int timer = 0;
    const int timer_re = 4;

    char time_str[20];

    bool running = true;

    // the main loop: update, draw, sleep
    while (running) {
        int key;

        switch (key = wgetch(stdscr)) {
        case KEY_esc:
        case KEY_q:
            running = false;
            break;
        // if we move here we also need to redraw the screen, hence the call to
        // erase() which isn't called normally
        case KEY_LEFT:
        case KEY_h:
            erase();
            if (((dimensions.x - 1) > 0))
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
            if (!((dimensions.y - 1) <= 0))
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

        // update clock every four ticks (1 second)
        timer++;
        if (timer == timer_re) {
            cur_time = time(NULL);
            *local_time = *localtime(&cur_time);

            // color for the clock, defined in init()
            attron(COLOR_PAIR(1));
            move(dimensions.y, dimensions.x);

            // create a string that holds the date and time
            sprintf(time_str, "%02d.%02d.%04d %02d:%02d:%02d",
                    local_time->tm_mday, local_time->tm_mon + 1,
                    local_time->tm_year + 1900, local_time->tm_hour,
                    local_time->tm_min, local_time->tm_sec);

            // draw the date and time to the screen
            addstr(time_str);

            // use a complicated mess of functions to draw the last and next
            // numbers to the screen above and below the date
            attron(COLOR_PAIR(2));

            last_next_day(                  dimensions.y, dimensions.x +  0, local_time->tm_mday, local_time->tm_mon + 1, local_time->tm_year + 1900);
            last_next_mon(                  dimensions.y, dimensions.x +  3, local_time->tm_mon + 1);
            last_next_year(                 dimensions.y, dimensions.x +  6, local_time->tm_year + 1900);
            last_next_hours_mins_or_seconds(dimensions.y, dimensions.x + 11, local_time->tm_hour, 24);
            last_next_hours_mins_or_seconds(dimensions.y, dimensions.x + 14, local_time->tm_min, 60);
            last_next_hours_mins_or_seconds(dimensions.y, dimensions.x + 17, local_time->tm_sec, 60);

            refresh();

            timer = 0;

            usleep(U_INTERVALL);
        }
    }

    endwin();

    return 0;
}

// cleanly exit ncurses
void finish(int sig)
{
    endwin();
    if (sig == SIGSEGV)
        printf("Segfault\n");
    exit(0);
}

// Functions to draw the last and next time unit above and below the current one

void last_next_hours_mins_or_seconds(int y, int x, int unit, int max)
{
    int next = unit + 1;
    int last = unit - 1;

    if (next >= max)
        next = 0;
    else if (last <= -1)
        last = max - 1;

    addstr_last_next(y, x, next, last, 2);
}

void last_next_day(int y, int x, int day, int mon, int year)
{
    int next = day + 1;
    int last = day - 1;

    // days depend on the month
    if (next > days_in_month(mon, year)) {
        next = 1;
    }

    if (last <= 0) {
        if(mon - 1 <= 0) {
            // Last day is in December of last year
            last = days_in_month(12, year - 1);
        } else {
            last = days_in_month(mon - 1, year);
        }
    }

    addstr_last_next(y, x, next, last, 2);
}

void last_next_mon(int y, int x, int mon)
{
    int next = mon + 1;
    int last = mon - 1;

    // months are shifted, since they don't start with 0
    if (next == 13)
        next = 1;
    if (last == 0)
        last = 12;

    addstr_last_next(y, x, next, last, 2);
}

void last_next_year(int y, int x, int year)
{
    int next = year + 1;
    int last = year - 1;

    addstr_last_next(y, x, next, last, 4);
}

void addstr_last_next(int y, int x, int next, int last, int len)
{
    assert(len == 2 || len == 4);

    char s_next[len + 1];
    char s_last[len + 1];

    // convert the last and next units to strings with sprintf()
    sprintf(s_next, len == 2 ? "%02d" : "%04d", next);
    sprintf(s_last, len == 2 ? "%02d" : "%04d", last);

    // draw the strings to the ncurses screen
    move(y + 1, x);
    addstr(s_next);

    move(y - 1, x);
    addstr(s_last);
}

// Return the days that a month has
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
        return 0;
    }
}
