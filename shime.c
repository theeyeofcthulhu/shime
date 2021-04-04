#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>
#include <string.h>

#include "util.h"
#include "shime.h"

int main(){
	init();
	loop();

	return 0;
}

void init(){
	signal(SIGINT, finish);

	initscr();
	nonl();
	curs_set(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
}

void loop(){
	time_t t_placeholder = time(NULL);
	struct tm *local_time = localtime(&t_placeholder);
	while(1){
		update_time(local_time);

		draw(local_time);

		sleep(1);
	}
}

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
}

void last_and_next_year(int y, int x, int unit){	
	unit += 1900;

	move(y + 1, x);
	int i_next = unit + 1;
	char *s_next = (char*)malloc(4 * sizeof(char));
	sprintf(s_next, "%4d", i_next);
	addstr(s_next);
	
	move(y - 1, x);
	int i_last = unit - 1;
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
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
}

void draw(struct tm *local_time){
	erase();
	
	attron(COLOR_PAIR(1));
	move(3, 5);

	char *s_local_time = (char*)malloc(19 * sizeof(char));
	sprintf(s_local_time, "%2d.%2d.%4d %2d:%2d:%2d",
			local_time->tm_mday,
			local_time->tm_mon + 1,
			local_time->tm_year + 1900,
			local_time->tm_hour,
			local_time->tm_min,
			local_time->tm_sec);
	strreplace(s_local_time, ' ', '0'); 
	s_local_time[10] = ' ';

	addstr(s_local_time);
	
	attron(COLOR_PAIR(2));
	last_and_next(3, 22, local_time->tm_sec, 60);
	last_and_next(3, 19, local_time->tm_min, 60);
	last_and_next(3, 16, local_time->tm_hour, 24);
	last_and_next_mon(3, 8, local_time->tm_mon + 1);
	last_and_next_year(3, 11, local_time->tm_year);
	last_and_next_day(3, 5, local_time->tm_mday, local_time->tm_mon + 1);

	refresh();
}
