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

/* TODO: specify strftime() format via command line */

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

#include <SDL2/SDL.h>

/* Ascii table keys */
#define KEY_q 113
#define KEY_h 104
#define KEY_j 106
#define KEY_k 107
#define KEY_l 108
#define KEY_esc 27

#define NANO_INTERVAL 10000000

#define HOUR_IN_SECS 60 * 60

#define SOUND_PATH "Bell, Counter, A.wav"

/* Stores format to be read by strftime() */
typedef struct {
    char *fmt;
    char *last_next;
} DateTimeFormat;

const DateTimeFormat de = {
    .fmt = "%d.%m.%Y %H:%M:%S",
    .last_next = "%d %m %Y %H %M %S",
};

const DateTimeFormat us = {
    .fmt = "%m/%d/%Y %H:%M:%S",
    .last_next = "%m %d %Y %H %M %S",
};

const DateTimeFormat timer_fmt = {
    .fmt = "%H:%M:%S",
    .last_next = "%H %M %S",
};

typedef struct {
    int y;
    int x;
    int height;
    int width;
} Dimensions;

typedef struct {
    time_t start;
    int mins;
    int secs;
} Timer;

/* SDL Audio variables (for timer) */
Uint8 *audio_pos;
Uint32 audio_len;

DateTimeFormat strtimeformat(char *str);

void finish(int sig);

void audio_callback(void *userdata, Uint8 *stream, int len);

void get_last_next_hours_mins_or_seconds(int n, int max, int *last, int *next);
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next);

int days_in_month(int mon, int year);

DateTimeFormat strtimeformat(char *str)
{
    if (strcmp(str, "de") == 0) {
        return de;
    } else if (strcmp(str, "us") == 0) {
        return us;
    } else {
        fprintf(stderr, "Invalid format: '%s'\n", str);
        exit(1);
    }
}

/* Cleanly exit ncurses */
void finish(int sig)
{
    endwin();
    if (sig == SIGSEGV) {
        fprintf(stderr, "Segfault\n");
        exit(1);
    }
    exit(0);
}

/* For SDL2 audio playing: copy current audio data (audio_pos) into stream */
void audio_callback(void *userdata, Uint8 *stream, int len)
{
    (void)userdata;

    if (audio_len == 0)
        return;

    len = len > (int)audio_len ? (int)audio_len : len;
    SDL_memcpy(stream, audio_pos, len);

    audio_pos += len;
    audio_len -= len;
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

/* Calculate the last and the next time unit for every unit in cur
 * Store in *last and *next */
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next)
{
    int year = cur.tm_year + 1900;
    int mon = cur.tm_mon + 1;

    /* Days */
    if (cur.tm_mday + 1 > days_in_month(mon, year)) {
        next->tm_mday = 1;
    } else {
        next->tm_mday = cur.tm_mday + 1;
    }
    if (cur.tm_mday - 1 <= 0) {
        if (mon - 1 <= 0) {
            /* Last day is in December of last year */
            last->tm_mday = days_in_month(12, year - 1);
        } else {
            last->tm_mday = days_in_month(mon - 1, year);
        }
    } else {
        last->tm_mday = cur.tm_mday - 1;
    }

    /* Months */
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

    /* Years */
    next->tm_year = cur.tm_year + 1;
    last->tm_year = cur.tm_year - 1;

    get_last_next_hours_mins_or_seconds(cur.tm_hour, 24, &last->tm_hour,
                                        &next->tm_hour); /* Hours */
    get_last_next_hours_mins_or_seconds(cur.tm_min, 60, &last->tm_min,
                                        &next->tm_min); /* Minutes */
    get_last_next_hours_mins_or_seconds(cur.tm_sec, 60, &last->tm_sec,
                                        &next->tm_sec); /* Seconds */
}

/* Return the days a month has */
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
        /* Leap year: https://en.wikipedia.org/wiki/Leap_year#Algorithm */
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
    Timer timer;

    Uint8 *wav_buffer;

    bool using_timer = false;

    int arg;
    while ((arg = getopt(argc, argv, "hf:t:")) != -1) {
        switch (arg) {
        case 'h':
            /* Help page */
            printf("shime Copyright (C) 2021 eyeofcthulhu\n\n"
                   "options:\n"
                   "-h: display help\n"
                   "-f [de us]: change the time format\n"
                   "-u: display usage\n\n"
                   "-t [NUMBER]: timer in minutes\n"
                   "controls\n"
                   "vim keys - hjkl or arrow keys: move around\n"
                   "q or esc: exit\n");
            return 0;
            break;
        case 'f':
            if (using_timer) {
                fprintf(stderr, "Cannot specify format when '-t' is given\n");
                return 1;
            }
            format = strtimeformat(optarg);
            break;
        case 't':
        {
            /* Read optarg into number and intialize SDL to play the sound */

            using_timer = true;
            format = timer_fmt;

            /* Read minutes (up to ':') */
            char *end;
            timer.mins = strtol(optarg, &end, 0);
            if (*end != ':') {
                fprintf(stderr,
                        "Expected string in the format: MINUTES:SECONDS\n");
                return 1;
            }

            /* Read seconds */
            timer.secs = strtol(end + 1, &end, 0);
            /* If *end is '\0', the whole string was read as a number (see man
             * strtol(3)) */
            if (*end != '\0') {
                fprintf(stderr,
                        "Expected string in the format: MINUTES:SECONDS\n");
                return 1;
            }
            if (timer.mins < 0 || timer.secs < 0) {
                fprintf(stderr, "Timer cannot be negative\n");
                return 1;
            }

            /* Initialize SDL for playing sound after timer has ended */
            if (SDL_Init(SDL_INIT_AUDIO) < 0) {
                fprintf(stderr, "Could not initialize SDL\n");
                return 1;
            }

            Uint32 wav_length;
            SDL_AudioSpec wav_spec;
            if (SDL_LoadWAV(SOUND_PATH, &wav_spec, &wav_buffer, &wav_length) ==
                NULL) {
                fprintf(stderr, "Could not open audio file '%s'\n", SOUND_PATH);
                return 1;
            }
            wav_spec.callback = audio_callback;
            wav_spec.userdata = NULL;

            audio_pos = wav_buffer;
            audio_len = wav_length;

            if (SDL_OpenAudio(&wav_spec, NULL) < 0) {
                fprintf(stderr, "Could not open audio: %s\n", SDL_GetError());
                return 1;
            }
            break;
        }
        case '?':
        default:
            return 1;
            break;
        }
    }

    /* Ncurses init logic */

    /* On interrupt (Ctrl+c) exit */
    signal(SIGINT, finish);
    /* On segfault: endwin() to avoid messing up terminal */
    signal(SIGSEGV, finish);

    initscr();

    /* Return key doesn't become newline */
    nonl();

    /* Disable cursor */
    curs_set(0);

    /* Allows Ctrl+c to quit the program */
    cbreak();

    /* Don't echo the the getch() chars onto the screen */
    noecho();

    /* Getch() doesn't wait for input and just returns ERR if no key is pressed */
    nodelay(stdscr, true);

    /* Enable keypad (for arrow keys) */
    keypad(stdscr, true);

    /* Color support */
    if (!has_colors()) {
        endwin();
        fprintf(stderr, "Your terminal does not support color\n");
        return 1;
    }

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_BLUE, COLOR_BLACK);

    /* Init dimensions */
    Dimensions dimensions;
    getmaxyx(stdscr, dimensions.height, dimensions.width);

    dimensions.y = dimensions.height / 2;
    dimensions.x = dimensions.width / 2 - (strlen(format.fmt) / 2);

    /* Initialize and get the localtime */
    time_t cur_time = time(NULL);
    struct tm *local_time = localtime(&cur_time);

    if (using_timer)
        timer.start = cur_time + timer.mins * 60 + timer.secs;

    /* Timer on with to update the clock */
    const int redraw_reset = 50;
    int redraw_timer = redraw_reset - 1; /* Start timer almost at reset so we draw instantly */

    /* How much we want to sleep every tick */
    const struct timespec request = {0, NANO_INTERVAL};

    char buf[128];

    bool running = true;

    bool need_to_erase = false;

    /* The main loop: update, draw, sleep */
    while (running) {
        switch (wgetch(stdscr)) {
        case KEY_esc:
        case KEY_q:
            running = false;
            break;
        /* If we move here we also need to redraw the screen, hence the call to
         * erase() which isn't called normally, as the numbers just get drawn
         * over */
        case KEY_LEFT:
        case KEY_h:
            need_to_erase = true;
            if ((dimensions.x - 1) > 0)
                dimensions.x -= 1;
            break;
        case KEY_DOWN:
        case KEY_j:
            need_to_erase = true;
            if ((dimensions.y + 1) < dimensions.height - 1)
                dimensions.y += 1;
            break;
        case KEY_UP:
        case KEY_k:
            need_to_erase = true;
            if ((dimensions.y - 1) > 0)
                dimensions.y -= 1;
            break;
        case KEY_RIGHT:
        case KEY_l:
            need_to_erase = true;
            if ((dimensions.x + 1) < dimensions.width - 19)
                dimensions.x += 1;
            break;
        default:
            break;
        }

        /* Update clock every four ticks */
        redraw_timer++;
        if (redraw_timer == redraw_reset) {
            if (need_to_erase) {
                erase();
                need_to_erase = false;
            }

            cur_time = time(NULL);

            if (using_timer) {
                /* How far away are we from reaching start? */
                cur_time = timer.start - cur_time;

                /* Finished */
                if (cur_time < 0) {
                    /* Play the 'ding' sound */
                    SDL_PauseAudio(0);
                    while (audio_len > 0) {
                        SDL_Delay(100);
                    }
                    SDL_CloseAudio();
                    SDL_FreeWAV(wav_buffer);

                    endwin();
                    printf("Timer over\n");
                    return 0;
                }

                /* The timer is relative to 1970-1-1 00:00:00 and localtime() */
                /* would change the hour value according to the timezone. */
                *local_time = *gmtime(&cur_time);
            } else {
                *local_time = *localtime(&cur_time);
            }

            /* Color for the clock, defined in init() */
            attron(COLOR_PAIR(1));

            strftime(buf, sizeof(buf), format.fmt, local_time);

            /* Draw the date and time to the screen */
            mvaddstr(dimensions.y, dimensions.x, buf);

            /* Draw the previous and next times */
            struct tm last, next;
            get_last_next_time(*local_time, &last, &next);

            attron(COLOR_PAIR(2));

            strftime(buf, sizeof(buf), format.last_next, &last);
            mvaddstr(dimensions.y - 1, dimensions.x, buf);

            strftime(buf, sizeof(buf), format.last_next, &next);
            mvaddstr(dimensions.y + 1, dimensions.x, buf);

            refresh();

            redraw_timer = 0;
        }
        thrd_sleep(&request, NULL);
    }

    endwin();

    return 0;
}
