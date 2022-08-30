#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

int errorCount = 0;
FILE *dbf;
char fbuff[100];
char ibuff[20];
char convbuff[20];

// returns a number between 1 and max
int Random(int max) {
   return (rand()%max) + 1;
}

// Log Single errors
void LogError(char *msg) {
   FILE *err = fopen("ErrorLog.txt", "a");
   printf("%s\n", msg);
   fclose(err);
   errorCount++;
}

// Log Errors with two parameters
void LogError2(const char *msg1, const char *msg2) {
   FILE *err = fopen("ErrorLog.txt", "a");
   fprintf(err, "%s %s\n", msg1, msg2);
   fclose(err);
   errorCount++;
}

void l2(char *loc, char *msg) {
#if 0
   fprintf(dbf, "%s,%s\n", loc, msg);
#endif
}

void l(char *loc) {
#if 0
   fprintf(dbf, "%s\n", loc);
#endif
}

void ln(char *loc, char *msg, int n) {
#if 0
   fprintf(dbf, "%s,%s,%s\n", loc, msg, sltoa(n));
#endif
}

char *sltoa(int n) {
   snprintf(convbuff, sizeof(convbuff), "%d", n);
   return convbuff;
}

void InitLogging(char *filename) {
   dbf = fopen("BigLog.txt", "wt");
}

void CloseLogging() {
   fclose(dbf);
}
