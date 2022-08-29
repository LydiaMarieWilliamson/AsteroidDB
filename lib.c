#ifndef lib
#include <stdio.h>
#include <stdlib.h>
#include "lib.h"
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#define lib
#endif

int errorCount = 0;
FILE * dbf;
char fbuff[100];
char ibuff[20];
char convbuff[20];

// returns a number between 1 and max 
int Random(int max) {
	return (rand() % max) + 1;
}

// Log Single errors
void LogError(char * msg) {
	FILE * err;
	int error = fopen_s(&err, "errorlog.txt", "a");
	printf("%s\n", msg);
	fclose(err);
	errorCount++;
}

// Log Errors with two parameters
void LogError2(const char * msg1, const char * msg2) {
	FILE * err;
	int error = fopen_s(&err, "errorlog.txt", "a");
	fprintf_s(err, "%s %s\n", msg1, msg2);
	fclose(err);
	errorCount++;
}

void l2(char * loc, char * msg) {
	//fprintf_s(dbf, "%s,%s\n", loc, msg);
}

void l(char * loc) {
	//fprintf_s(dbf, "%s\n", loc);
}

void ln(char * loc, char * msg, int n) {
	//fprintf_s(dbf, "%s,%s,%s\n", loc, msg, sltoa(n));
}

char * sltoa(int n) {
	_ltoa_s( n, convbuff, sizeof(convbuff),10);
	return convbuff;
} 

void InitLogging(char * filename) {
	int error = fopen_s(&dbf, "biglog.txt", "wt");
}

void CloseLogging() {
	fclose(dbf);
}

