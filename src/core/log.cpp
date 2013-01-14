#include "log.h"

#include <ctime>
#include <cstdio>
#include <cstdarg>

void logToFile(char* fmt, ...)
{
   FILE * logFile;
   logFile = fopen(LOG_FILENAME, "a");
   va_list args;
   va_start(args, fmt);
   vfprintf(logFile, fmt, args);
   va_end(args);
   fclose(logFile);
}