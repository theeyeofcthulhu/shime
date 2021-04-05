#include "shime.h"

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

	//grey color
	init_color(COLOR_BLUE, 200, 200, 200);	
	init_pair(2, COLOR_BLUE, COLOR_BLACK);
}

void loop(){
	time_t t_placeholder = time(NULL);
	struct tm *local_time = localtime(&t_placeholder);

	//used to offset the date and time when moving with arrow keys
	int *x = (int*)malloc(sizeof(int));
	*x = 5;
	int *y = (int*)malloc(sizeof(int));
	*y = 3;

	int timer = 0;
	const int timer_re = 4;

	//the main loop: update, draw, sleep
	while(1){
		key_handling(x, y);

		timer++;
		if(timer == timer_re){
			update_time(local_time);
			draw(local_time, x, y);
			timer = 0;
		}

		sleep(0.25);
	}
}

//gets the localtime, stores it in the given pointer
void update_time(struct tm *local_time){
	time_t t_placeholder = time(NULL);
	*local_time = *localtime(&t_placeholder);
}

void finish(int sig){
	endwin();
	exit(0);
}

void last_and_next(int y, int x, int unit, int base, int mon, int mode){	
	int i_next = unit + 1;
	int i_last = unit - 1;

	int i_days_in_month = days_in_month(mon);

	switch(mode){	
	case MODE_min_h:
		if(i_next == base)
			i_next = 0;
		if(i_last == -1)
			i_last = base - 1;
		break;
	case MODE_day:
		if(i_next >= i_days_in_month)
			i_next = 0;
		if(i_last == -1)
			i_last = days_in_month(mon - 1);
		break;
	case MODE_mon:
		if(i_next == 13)
			i_next = 1;
		if(i_last == 0)
			i_last = 12;
		break;
	case MODE_year:
		break;
	}

	char *s_next;

	char *s_last;

	if(mode != MODE_year){
		s_next = (char*)malloc(2 * sizeof(char));
		sprintf(s_next, "%2d", i_next);
		s_last = (char*)malloc(2 * sizeof(char));
		sprintf(s_last, "%2d", i_last);
	}else{
		s_next = (char*)malloc(4 * sizeof(char));
		sprintf(s_next, "%4d", i_next);
		s_last = (char*)malloc(4 * sizeof(char));
		sprintf(s_last, "%4d", i_last);
	}

	move(y + 1, x);
	addstr(s_next);

	//move call is necessary here because i think addstr() moves the cursor
	if(i_next < 10){
		move(y + 1, x);
		addstr("0");
	}	
	free(s_next);
	
	move(y - 1, x);
	addstr(s_last);
	if(i_last < 10){
		move(y - 1, x);
		addstr("0");
	}	
	free(s_last);
}

void key_handling(int *x, int *y){
	int key;

	//key handling with wget
	switch(key = wgetch(stdscr)){
	case KEY_esc:
		finish(0);
		break;
	case KEY_q:
		finish(0);
		break;

	//if we move here we also need to redraw the screen, hence the call to erase() which isn't called normally
	case KEY_LEFT:
	case KEY_h:
		erase();
		if(!((*x - 1) < 0))
			*x -= 1;
		break;
	case KEY_DOWN:
	case KEY_j:
		erase();
		*y += 1;
		break;
	case KEY_UP:
	case KEY_k:
		erase();
		if(!((*y - 1) <= 0))
			*y -= 1;
		break;
	case KEY_RIGHT:
	case KEY_l:
		erase();
		*x += 1;
		break;
	default:
		break;
	}
}

void draw(struct tm *local_time, int *x, int *y){
	attron(COLOR_PAIR(1));
	move(*y, *x);

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
	last_and_next(*y, *x + 0, local_time->tm_mday, NOARG, local_time->tm_mon + 1, MODE_day);
	last_and_next(*y, *x + 3, local_time->tm_mon + 1, NOARG, NOARG, MODE_mon);
	last_and_next(*y, *x + 6, local_time->tm_year + 1900, NOARG, NOARG, MODE_year);
	last_and_next(*y, *x + 11, local_time->tm_hour, 24, NOARG, MODE_min_h);
	last_and_next(*y, *x + 14, local_time->tm_min, 60, NOARG, MODE_min_h);
	last_and_next(*y, *x + 17, local_time->tm_sec, 60, NOARG, MODE_min_h);

	refresh();
}
