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

#define BUF_SZ 128

#define NANO_INTERVAL 10000000

#define SOUND_PATH "Bell, Counter, A.wav"
#ifndef BUILD_SOUND_PATH
#define BUILD_SOUND_PATH SOUND_PATH /* Will be defined by make, sound path in install folder */
#endif

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
    int secs;
} Timer;

enum ClockType {
    CLOCK,
    TIMER,
    STOPWATCH,
};

/* SDL Audio variables (for timer) */
Uint8 *audio_pos;
Uint32 audio_len;

bool print_elapsed_on_exit = false;
time_t global_start;

DateTimeFormat strtimeformat(char *str);
int strtosecs(char* str);

void finish(int sig);

void getmaxyx_and_go_to_middle(Dimensions *d, int clock_len);

void audio_callback(void *userdata, Uint8 *stream, int len);

void get_last_next_hours_mins_or_seconds(int n, int max, int *last, int *next);
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next, bool timer);

void timertime(time_t in, struct tm *out);
void timerformat(char *out, size_t max, const char *format, const struct tm *in);

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

/* Valid strings:
 * SECONDS as int
 * MINUTES:SECONDS
 * HOURS:MINUTES:SECONDS 
 *
 * returns -1 on failure */
int strtosecs(char* str)
{
    int units[3] = {0}; /* {HOURS, MINUTES, SECONDS} */
    char *end, *saved;

    /* Read hours (up to ':') */
    units[0] = strtol(str, &end, 0);
    if (*end == '\0') {
        units[2] = units[0];
        units[0] = 0;
        goto return_calculation;
    }
    if (*end != ':' || (*end == ':' && *(end + 1) == '\0') || end == str) {
        return -1;
    }
    saved = end + 1;

    /* Read minutes */
    units[1] = strtol(saved, &end, 0);
    /* If *end is '\0', the whole string was read as a number (see man
     * strtol(3)) */
    if (*end == '\0') {
        units[2] = units[1];
        units[1] = units[0];
        units[0] = 0;
        goto return_calculation;
    }
    if (*end != ':' || (*end == ':' && *(end + 1) == '\0') || end == saved) {
        return -1;
    }
    saved = end + 1;

    /* Read seconds */
    units[2] = strtol(saved, &end, 0);
    if (*end != '\0' || end == saved) {
        return -1;
    }

return_calculation:
    if (units[0] < 0 || units[1] < 0 || units[2] < 0) {
        return -1;
    }

    return units[0] * 60 * 60 + units[1] * 60 + units[2];
}

/* Cleanly exit ncurses */
void finish(int sig)
{
    endwin();
    if (print_elapsed_on_exit) {
        time_t elapsed =  time(NULL) - global_start;
        char buf[BUF_SZ];
        strftime(buf, sizeof(buf), "Elapsed: %H:%M:%S\n", gmtime(&elapsed));
        printf("%s", buf);
    }
    if (sig == SIGSEGV) {
        fprintf(stderr, "Segfault\n");
        exit(1);
    }
    exit(0);
}

/* Gets ncurses boundaries and sets y and x to middle of them */
void getmaxyx_and_go_to_middle(Dimensions *d, int clock_len)
{
    getmaxyx(stdscr, d->height, d->width);

    d->y = d->height / 2;
    d->x = d->width / 2 - clock_len / 2;
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
void get_last_next_time(struct tm cur, struct tm *last, struct tm *next, bool timer)
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

    if (timer) {
        next->tm_hour = cur.tm_hour + 1;
        last->tm_hour = cur.tm_hour - 1 >= 0 ? cur.tm_hour - 1 : 99;
    } else {
        get_last_next_hours_mins_or_seconds(cur.tm_hour, 24, &last->tm_hour,
                                            &next->tm_hour); /* Hours */
    }

    get_last_next_hours_mins_or_seconds(cur.tm_min, 60, &last->tm_min,
                                        &next->tm_min); /* Minutes */
    get_last_next_hours_mins_or_seconds(cur.tm_sec, 60, &last->tm_sec,
                                        &next->tm_sec); /* Seconds */
}

/* Like localtime(3) but for a timer which doesn't want tm_hour bound from 0 to 23 */
void timertime(time_t in, struct tm *out)
{
    out->tm_mday = out->tm_mon = out->tm_year = out->tm_wday = out->tm_yday = out->tm_isdst = 0;

    out->tm_sec = in % 60;
    out->tm_min = (in % (60 * 60)) / 60;
    out->tm_hour = in / (60 * 60);
}

/* If *format begins with %H: print the hour first and then call strftime
 * Otherwise: just pass to strftime 
 * Have this because strftime does not allow tm_hour > 23 */
void timerformat(char *out, size_t max, const char *format, const struct tm *in)
{
    if (format[0] == '%' && format[1] == 'H') {
        int printed = snprintf(out, max, "%02d", in->tm_hour);

        char *new_out = out + printed;
        max -= printed;

        strftime(new_out, max, format + 2, in);
    } else {
        strftime(out, max, format, in);
    }
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
    int clock_len;

    Uint8 *wav_buffer;

    enum ClockType mode = CLOCK;
    Timer timer;

    int mutually_exclusive_opts = 0;
    int arg;
    while ((arg = getopt(argc, argv, "hf:t:ie")) != -1) {
        switch (arg) {
        case 'h':
        {
            /* Help page */
            printf("shime Copyright (C) 2021 theeyeofcthulhu on GitHub\n"
                   "\n"
                   "options:\n"
                   "-h:           Display this message and exit\n"
                   "-f [de us]:   Change the time format\n"
                   "-t [TIME]:    Start a timer which plays a sound on finish.\n"
                   "              TIME is a string in this format: HOURS:MINUTES:SECONDS.\n"
                   "              HOURS or MINUTES and HOURS can be left out.\n"
                   "-i:           Incremental timer\n"
                   "-e:           Print elapsed time at exit\n"
                   "\n"
                   "controls\n"
                   "vim keys - hjkl or arrow keys: move around\n"
                   "q or esc: exit\n");
            return 0;
            break;
        }
        case 'f':
        {
            mutually_exclusive_opts++;

            format = strtimeformat(optarg);
            break;
        }
        case 't':
        {
            /* Read optarg into number and intialize SDL to play the sound */
            mutually_exclusive_opts++;

            mode = TIMER;
            format = timer_fmt;

            timer.secs = strtosecs(optarg);
            if(timer.secs <= 0) {
                fprintf(stderr, 
                        "Failed to parse '%s' to a timer. "
                        "Expected HOURS:MINS:SECONDS (or MINS:SECONDS / SECONDS) as a positive integer.\n", 
                        optarg);
                return 1;
            }

            /* Initialize SDL for playing sound after timer has ended */
            if (SDL_Init(SDL_INIT_AUDIO) < 0) {
                fprintf(stderr, "Could not initialize SDL\n");
                return 1;
            }

            Uint32 wav_length;
            SDL_AudioSpec wav_spec;
            /* Try to open in current folder and then in install folder */
            if (SDL_LoadWAV(SOUND_PATH, &wav_spec, &wav_buffer, &wav_length) == NULL &&
                SDL_LoadWAV(BUILD_SOUND_PATH, &wav_spec, &wav_buffer, &wav_length) == NULL) {
                fprintf(stderr, "Could not open audio file: %s\n", SDL_GetError());
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
        case 'i':
        {
            mutually_exclusive_opts++;

            mode = STOPWATCH;
            format = timer_fmt;
            break;
        }
        case 'e':
        {
            print_elapsed_on_exit = true;
            break;
        }
        case '?':
        default:
            return 1;
            break;
        }
    }
    if(mutually_exclusive_opts > 1) {
        fprintf(stderr, "Specified mutually exclusive options\n");
        return 1;
    }

    clock_len = strlen(format.fmt);

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
    getmaxyx_and_go_to_middle(&dimensions, clock_len);

    /* Initialize and get the localtime */
    time_t cur_time = time(NULL);
    struct tm *local_time = localtime(&cur_time);

    if (mode == TIMER)
        timer.start = cur_time + timer.secs;
    else if (mode == STOPWATCH)
        timer.start = cur_time;

    global_start = cur_time;

    /* Timer on with to update the clock */
    const int redraw_reset = 50;
    int redraw_timer = redraw_reset - 1; /* Start timer almost at reset so we draw instantly */

    /* How much we want to sleep every tick */
    const struct timespec sleep_request = {0, NANO_INTERVAL};

    char buf[BUF_SZ];

    bool running = true;
    bool need_to_erase = false;
    bool is_timer = mode == TIMER || mode == STOPWATCH;

    /* The main loop: update, draw, sleep */
    while (running) {
        switch (getch()) {
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
            if (dimensions.x - 1 > 0)
                dimensions.x -= 1;
            break;
        case KEY_DOWN:
        case KEY_j:
            need_to_erase = true;
            if (dimensions.y + 1 < dimensions.height - 1)
                dimensions.y += 1;
            break;
        case KEY_UP:
        case KEY_k:
            need_to_erase = true;
            if (dimensions.y - 1 > 0)
                dimensions.y -= 1;
            break;
        case KEY_RIGHT:
        case KEY_l:
            need_to_erase = true;
            if (dimensions.x + 1 < dimensions.width - clock_len)
                dimensions.x += 1;
            break;
        case KEY_RESIZE: 
            need_to_erase = true;
            getmaxyx_and_go_to_middle(&dimensions, clock_len);
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

            switch(mode) {
            case TIMER:
            {
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

                    finish(0);
                }

                /* The timer is relative to 1970-1-1 00:00:00 and localtime() */
                /* would change the hour value according to the timezone. */
                timertime(cur_time, local_time);
                break;
            }
            case STOPWATCH:
            {
                cur_time = cur_time - timer.start;
                timertime(cur_time, local_time);
                break;
            }
            case CLOCK:
            {
                *local_time = *localtime(&cur_time);
                break;
            }
            }

            /* Color for the clock, defined in init() */
            attron(COLOR_PAIR(1));

            timerformat(buf, sizeof(buf), format.fmt, local_time);

            /* Draw the date and time to the screen */
            mvaddstr(dimensions.y, dimensions.x, buf);

            /* Draw the previous and next times */
            struct tm last, next;
            get_last_next_time(*local_time, &last, &next, is_timer);

            attron(COLOR_PAIR(2));

            timerformat(buf, sizeof(buf), format.last_next, &last);
            mvaddstr(dimensions.y - 1, dimensions.x, buf);

            timerformat(buf, sizeof(buf), format.last_next, &next);
            mvaddstr(dimensions.y + 1, dimensions.x, buf);

            refresh();

            redraw_timer = 0;
        }
        thrd_sleep(&sleep_request, NULL);
    }

    finish(0);
}
