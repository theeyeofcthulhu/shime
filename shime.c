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

#include "shime.h"

struct dimensions{
	int y;
	int x;
	int height;
	int width;
};

int main(int argc, char **argv){
    //argparse with getopt()
    int arg;
    while((arg = getopt(argc, argv, "hu")) != -1){
        switch(arg)
        {
        case 'h':
			//help page
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

	//ncurses init logic
    //on interrupt (Ctrl+c) exit
	signal(SIGINT, finish);
	signal(SIGSEGV, finish);

    //init
	initscr();

    //return key doesn't become newline
	nonl();

    //disable curosr
	curs_set(0);

    //allows Ctrl+c to quit the program
	cbreak();

    //don't echo the the getch() chars onto the screen
	noecho();

    //getch() doesn't wait for input and just returns ERR if no key is pressed
	nodelay(stdscr, true);

    //enable keypad (for arrow keys)
	keypad(stdscr, true);

    //color support
	if(!has_colors())
		finish(0);

	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLACK);

	//grey color
	init_color(COLOR_BLUE, 200, 200, 200);	
	init_pair(2, COLOR_BLUE, COLOR_BLACK);

	//Init dimensions
	struct dimensions dimensions;
	getmaxyx(stdscr, dimensions.height, dimensions.width);
	dimensions.y = 3;
	dimensions.x = 5;

    //initialize and get the localtime
	time_t t_time = time(NULL);
	struct tm *local_time = localtime(&t_time);

    //timer on with to update the clock
	int timer = 0;
	const int timer_re = 4;

	//the main loop: update, draw, sleep
	while(1){
        //update keys every tick
		int key;

		//key handling with wget

		//exit keys apart from Ctrl+c
		switch(key = wgetch(stdscr)){
		case KEY_esc:
		case KEY_q:
			finish(0);
			break;

		//if we move here we also need to redraw the screen, hence the call to erase() which isn't called normally
		case KEY_LEFT:
		case KEY_h:
			erase();
			if(((dimensions.x - 1) > 0))
				dimensions.x -= 1;
			break;
		case KEY_DOWN:
		case KEY_j:
			erase();
			if((dimensions.y + 1) < dimensions.height - 1)
				dimensions.y += 1;
			break;
		case KEY_UP:
		case KEY_k:
			erase();
			if(!((dimensions.y - 1) <= 0))
				dimensions.y -= 1;
			break;
		case KEY_RIGHT:
		case KEY_l:
			erase();
			if((dimensions.x + 1) < dimensions.width - 19)
				dimensions.x += 1;
			break;
		default:
			break;
		}

        //update clock every four ticks (1 second)
		timer++;
		if(timer == timer_re){
			time_t t_time = time(NULL);
			*local_time = *localtime(&t_time);

			//color for the clock, defined in init()
			attron(COLOR_PAIR(1));
			move(dimensions.y, dimensions.x);

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
			draw_last_and_next(dimensions.y, dimensions.x + 0,		local_time->tm_mday, 		    NOARG, 	local_time->tm_mon + 1,	MODE_day);
			draw_last_and_next(dimensions.y, dimensions.x + 3,		local_time->tm_mon + 1, 	    NOARG, 	NOARG, 			        MODE_mon);
			draw_last_and_next(dimensions.y, dimensions.x + 6,		local_time->tm_year + 1900, 	NOARG, 	NOARG, 			        MODE_year);
			draw_last_and_next(dimensions.y, dimensions.x + 11, 	local_time->tm_hour, 		    24,	    NOARG, 			        MODE_min_h);
			draw_last_and_next(dimensions.y, dimensions.x + 14, 	local_time->tm_min, 		    60,	    NOARG, 			        MODE_min_h);
			draw_last_and_next(dimensions.y, dimensions.x + 17, 	local_time->tm_sec, 		    60,	    NOARG, 			        MODE_min_h);

			refresh();
					timer = 0;

			sleep(0.25);
		}
	}

	return 0;
}

//cleanly exit ncurses
void finish(int sig){
	endwin();
	if(sig == SIGSEGV)
		printf("Segfault\n");
	exit(0);
}

/*draws the last and next unit above and below the x and y coords
 *since this functions is spaghetti, here the meanings of the parameters:
 *  x, y: coords of the original unit
 *  unit: the value of the thing
 *  base: for minutes seconds and hours: the base value for the time unit (min and secs: 60, hours: 24)
 *  mon: for day: the current month, for calculating if the next day is, for example 31 or 0
 *  mode: the current mode of the thing, which are defined in the header for example MODE_day for day and MODE_mon for month
 */
void draw_last_and_next(int y, int x, int unit, int base, int mon, int mode){	
	int i_next = unit + 1;
	int i_last = unit - 1;

	int i_days_in_month;
	if(MODE_day)
		i_days_in_month = days_in_month(mon);

    //calculate the last and next units for different modes
	switch(mode){	
	case MODE_min_h:
        //hours and seconds are basic stuff
		if(i_next == base)
			i_next = 0;
		if(i_last == -1)
			i_last = base - 1;
		break;
	case MODE_day:
        //days depend on the month
		if(i_next >= i_days_in_month)
			i_next = 0;
		if(i_last == -1)
			i_last = days_in_month(mon - 1);
		break;
	case MODE_mon:
        //months are shifted, since they don't start with 0
		if(i_next == 13)
			i_next = 1;
		if(i_last == 0)
			i_last = 12;
		break;
	case MODE_year:
        //years don't need that since the are *infinite*
		break;
	}

	char *s_next, *s_last;

    //convert the last and next units to strings with sprintf()
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

    //draw the strings to the ncurses screen
	move(y + 1, x);
	addstr(s_next);
	//move call is necessary here because i think addstr() moves the cursor
	if(i_next < 10){
		move(y + 1, x);
		//add this 0 for numbers below ten so its like "01, 02, etc."
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

//return the days that a month, TODO: gap year february
int days_in_month(int mon){
	if(mon <= 0)
		mon = 12;
	
	switch(mon){
		case 1:
		     return 31;
		case 2:
		     return 28;
		case 3:
		     return 31;
		case 4:
		     return 30;
		case 5:
		     return 31;
		case 6:
		     return 30;
		case 7:
		     return 31;
		case 8:
		     return 31;
		case 9:
		     return 30;
		case 10:
		     return 31;
		case 11:
		     return 30;
		case 12:
		     return 31;
		default:
		     return 31;
	}
}

//replaces every char "original" with "replace"
void strreplace(char* string, char original, char replace){
	int length = strlen(string);
	for(int i = 0; i < length; i++){
		if(string[i] == original){
			string[i] = replace;
		}
	}	
}
