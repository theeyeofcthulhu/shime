#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

#include "util.h"

//ascii table keys
#define KEY_q 113
#define KEY_h 104
#define KEY_j 106
#define KEY_k 107
#define KEY_l 108
#define KEY_esc 27

#define MODE_min_h 0
#define MODE_day 1
#define MODE_mon 2
#define MODE_year 3

#define NOARG 0

void init(int *width, int *height);
void loop(int *width, int *height);
void key_handling(int *x, int *y, int *width, int *height);
void draw(struct tm *local_time, int xoff, int yoff);
void update_time(struct tm *local_time);
void finish(int sig);
void last_and_next(int y, int x, int unit, int base, int mon, int mode);
