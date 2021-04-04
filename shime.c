#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

#include "util.h"
#include "shime.h"

#define KEY_q 113
#define KEY_h 104
#define KEY_j 106
#define KEY_k 107
#define KEY_l 108
#define KEY_esc 27

int main(){
	init();
	loop();

	return 0;
}

//ncurses init logic
void init(){
	signal(SIGINT, finish);

	initscr();
	nonl();
	curs_set(0);

	cbreak();
	noecho();
	nodelay(stdscr, true);
	keypad(stdscr, true);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
}

void loop(){
	time_t t_placeholder = time(NULL);
	struct tm *local_time = localtime(&t_placeholder);

	//used to offset the date and time when moving with arrow keys
	int *xoff = (int*)malloc(sizeof(int));
	*xoff = 0;
	int *yoff = (int*)malloc(sizeof(int));
	*yoff = 0;

	//the main loop: update, draw, sleep
	while(1){
		update_time(local_time);

		draw(local_time, xoff, yoff);

		sleep(1);
	}
}

//gets the localtime, stores it in the given pointer
void update_time(struct tm *local_time){
	time_t t_placeholder = time(NULL);
	*local_time = *localtime(&t_placeholder);
	return;
}

void finish(int sig){
	endwin();
	exit(0);
}

void last_and_next(int y, int x, int unit, int base){	
	move(y + 1, x);
	int i_next = unit + 1;
	if(i_next == base)
		i_next = 0;
	char *s_next = (char*)malloc(2 * sizeof(char));
	sprintf(s_next, "%2d", i_next);
	addstr(s_next);
	if(i_next < 10){
		move(y + 1, x);
		addstr("0");
	}	
	free(s_next);
	
	move(y - 1, x);
	int i_last = unit - 1;
	if(i_last == -1)
		i_last = base - 1;
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
	if(i_last < 10){
		move(y - 1, x);
		addstr("0");
	}	
	free(s_last);
}

void last_and_next_year(int y, int x, int unit){	
	unit += 1900;

	move(y + 1, x);
	int i_next = unit + 1;
	char *s_next = (char*)malloc(4 * sizeof(char));
	sprintf(s_next, "%4d", i_next);
	addstr(s_next);
	free(s_next);
	
	move(y - 1, x);
	int i_last = unit - 1;
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
	free(s_last);
}

void last_and_next_day(int y, int x, int unit, int mon){	
	int i_days_in_month = days_in_month(mon);

	move(y + 1, x);
	int i_next = unit + 1;
	if(i_next >= i_days_in_month)
		i_next = 0;
	char *s_next = (char*)malloc(2 * sizeof(char));
	sprintf(s_next, "%2d", i_next);
	addstr(s_next);
	if(i_next < 10){
		move(y + 1, x);
		addstr("0");
	}	
	free(s_next);
	
	move(y - 1, x);
	int i_last = unit - 1;
	if(i_last == -1)
		i_last = days_in_month(mon - 1);
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
	if(i_last < 10){
		move(y - 1, x);
		addstr("0");
	}	
	free(s_last);
}

void last_and_next_mon(int y, int x, int unit){	
	move(y + 1, x);
	int i_next = unit + 1;
	if(i_next == 13)
		i_next = 1;
	char *s_next = (char*)malloc(2 * sizeof(char));
	sprintf(s_next, "%2d", i_next);
	addstr(s_next);
	if(i_next < 10){
		move(y + 1, x);
		addstr("0");
	}	
	free(s_next);
	
	move(y - 1, x);
	int i_last = unit - 1;
	if(i_last == 0)
		i_last = 12;
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
	if(i_last < 10){
		move(y - 1, x);
		addstr("0");
	}	
	free(s_last);
}

void draw(struct tm *local_time, int *xoff, int *yoff){

	//key handling with wget
	int key = wgetch(stdscr);
	switch(key){
	case KEY_esc:
		finish(0);
		break;
	case KEY_q:
		finish(0);
		break;

	//if we move here we also need to redraw the screen, hence the call to erase() which isn't called normally
	case KEY_LEFT:
		erase();
		*xoff -= 1;
		break;
	case KEY_DOWN:
		erase();
		*yoff += 1;
		break;
	case KEY_UP:
		erase();
		*yoff -= 1;
		break;
	case KEY_RIGHT:
		erase();
		*xoff += 1;
		break;

	//moving with vim bindings	
	case KEY_h:
		erase();
		*xoff -= 1;
		break;
	case KEY_j:
		erase();
		*yoff += 1;
		break;
	case KEY_k:
		erase();
		*yoff -= 1;
		break;
	case KEY_l:
		erase();
		*xoff += 1;
		break;
	default:
		break;
	}
	
	attron(COLOR_PAIR(1));
	move(*yoff + 3, *xoff + 5);

	//create a string that holds the date and time
	
	char *s_local_time = (char*)malloc(19 * sizeof(char));
	sprintf(s_local_time, "%2d.%2d.%4d %2d:%2d:%2d",
			local_time->tm_mday,
			local_time->tm_mon + 1,
			local_time->tm_year + 1900,
			local_time->tm_hour,
			local_time->tm_min,
			local_time->tm_sec);
	//here we replace all empty spaces with '0', so numbers always have a zero in front of them (function from util.h, don't know if theres a builtin for this)
	strreplace(s_local_time, ' ', '0');
	//the [10] character is the seperator between date and time so we need this to be a space
	s_local_time[10] = ' ';

	//draw the date and time to the screen
	addstr(s_local_time);

	free(s_local_time);
	
	//use a complicated mess of functions to draw the last and next numbers to the screen above and below the date
	attron(COLOR_PAIR(2));
	last_and_next(*yoff + 3, *xoff + 22, local_time->tm_sec, 60);
	last_and_next(*yoff + 3, *xoff + 19, local_time->tm_min, 60);
	last_and_next(*yoff + 3, *xoff + 16, local_time->tm_hour, 24);
	last_and_next_mon(*yoff + 3, *xoff + 8, local_time->tm_mon + 1);
	last_and_next_year(*yoff + 3, *xoff + 11, local_time->tm_year);
	last_and_next_day(*yoff + 3, *xoff + 5, local_time->tm_mday, local_time->tm_mon + 1);

	refresh();
}
