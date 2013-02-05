#include <matime.h>
#include <conprint.h>
#include <maassert.h>

static bool testDate(int year, int month, int day) {
	time_t ticks;
	{
		tm tim;
		tim.tm_hour = 0;
		tim.tm_min = 0;
		tim.tm_sec = 0;
		tim.tm_year = year;
		tim.tm_mon = month;
		tim.tm_mday = day;
		ticks = mktime( &tim );
	}
	{
		tm tim;
		split_time( ticks, &tim );
		if(tim.tm_year != year || tim.tm_mon != month || tim.tm_mday != day) {
			printf("ERROR: %02i:%02i:%02i => %02i:%02i:%02i\n",
				year, month, day, tim.tm_year, tim.tm_mon, tim.tm_mday);
			return false;
		}
		return true;
	}
}

int MAMain() GCCATTRIB(noreturn);
int MAMain() {
	printf("LT: %s\n", sprint_time(maLocalTime()));
	printf("Main test...\n");
	for(int y = 110; y >= 60; y--) {
		for(int m = 11; m >= 0; m--) {
			for(int d = 27; d >= 1; d--) {
				if(!testDate(y, m, d)) {
					FREEZE;
				}
			}
		}
	}
	printf("Main test success!\n");
	testDate(110, 10, 20);
	testDate(100, 10, 20);
	testDate(90, 10, 20);
	testDate(80, 10, 20);
	printf("Secondary test success!\n");
	FREEZE;
}
