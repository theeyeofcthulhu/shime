#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>

void update_time(struct tm *local_time);

void finish(int sig);

void last_and_next_base60(int x, int y, int unit);

void draw(struct tm *local_time);

int main(){
	
	signal(SIGINT, finish);

	initscr();
	nonl();
	curs_set(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);

	time_t t_placeholder = time(NULL);
	struct tm *local_time = localtime(&t_placeholder);
	while(1){
		update_time(local_time);

		draw(local_time);

		sleep(1);
	}
	return 0;
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

void last_and_next_base60(int y, int x, int unit){	
	attron(COLOR_PAIR(2));
	move(y + 1, x);
	int i_next = unit + 1;
	if(i_next == 60)
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
		i_last = 59;
	char *s_last = (char*)malloc(2 * sizeof(char));
	sprintf(s_last, "%2d", i_last);
	addstr(s_last);
	if(i_last < 10){
		move(y - 1, x);
		addstr("0");
	}	
}

void last_and_next_base24(int y, int x, int unit){	
	attron(COLOR_PAIR(2));
	move(y + 1, x);
	int i_next = unit + 1;
	if(i_next == 24)
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
		i_last = 23;
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
	addstr(asctime(local_time));
	
	last_and_next_base60(3, 22, local_time->tm_sec);
	last_and_next_base60(3, 19, local_time->tm_min);
	last_and_next_base24(3, 16, local_time->tm_hour);

	refresh();
}

