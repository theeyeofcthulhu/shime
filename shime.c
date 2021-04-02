#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <curses.h>
#include <signal.h>

void print_time(struct tm *local_time){
	system("clear");
	printf("Time:%s", asctime(local_time));
	sleep(1);
	return;
}

void update_time(struct tm *local_time){
	time_t t_placeholder = time(NULL);
	*local_time = *localtime(&t_placeholder);
	return;
}

static void finish(int sig){
	endwin();
	exit(0);
}

int main(){
	
	signal(SIGINT, finish);

	initscr();
	nonl();

	int x = 1;
	struct tm *local_time;
	time_t t_placeholder = time(NULL);
	local_time = localtime(&t_placeholder);
	//update_time(local_time);
	while(x == 1){
		//print_time(local_time);
		erase();

		move(1, 1);
		addstr(asctime(local_time));
		update_time(local_time);

		sleep(0.5);

		refresh();
		//x++;
	}
	return 0;
}
