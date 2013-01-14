#pragma once

#include "neogpc.h"

// our local logging file
#define LOG_FILENAME "NeoGPC.log"

// Only use the log if we've compiled with log support
#ifdef NEOGPC_LOG_ENABLED
   
void logToFile(char*,...);
#define LOG(fmt, ...) logToFile(fmt, __VA_ARGS__)

#else
#define LOG(x) //
#endif
