void init();
void loop();
void key_handling(int *xoff, int *off);
void draw(struct tm *local_time, int *xoff, int *yoff);
void update_time(struct tm *local_time);
void finish(int sig);
void last_and_next(int y, int x, int unit, int base, int mon, int mode);
