#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

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

struct dimensions;

void finish(int sig);
void draw_last_and_next(int y, int x, int unit, int base, int mon, int mode);
void strreplace(char* string, char original, char replace);
int days_in_month(int mon);
