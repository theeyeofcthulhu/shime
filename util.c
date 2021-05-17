#include "util.h"

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
