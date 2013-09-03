#pragma once

// Include the NeoGPC core
#include "..\core\neogpc.h"

// Do not include code if we are not using the debugger
#ifdef NEOGPC_DEBUGGER

// TLCS 900h Breakpoint
typedef struct _tlcs900hBreakpoint {
	unsigned int address;
	bool active;
	char buf[1024];
} tlcs900hBreakpoint;

// Debug states
enum {
	DEBUGGER_IDLE = 0,
	DEBUGGER_PAUSE,
	DEBUGGER_RESUME,
	DEBUGGER_STEP,
	DEBUGGER_OVER,
	DEBUGGER_INTO
};

// Size of instruction page (in MB)
#define MAX_INSTR_LEN 32

// Class for the TLCS900h Debugger
class tlcs900hdebugger
{
public:
	tlcs900hdebugger(void);
	~tlcs900hdebugger(void);

	tlcs900hBreakpoint * getBreakpointList();			// Return all breakpoints
	int setBreakpoint(unsigned int);		// Set a breakpoint at X location
	void setBreakpointName(const unsigned int, const char *);
	char * getBreakpointName(const unsigned int);
	void removeBreakpoint(int);	// Disable breakpoint (index)
	void enableBreakpoint(int);	// Enable breakpoint (index)
	void disableBreakpoint(int); // Disable breakpoint (index)
	void clearBreakpoints();	// clear all the breakpoints

	unsigned int decodeTlcs900h(unsigned long);	// decode the current PC, returning bytes eaten
	char * lastInstruction();		// get the last instruction that was decoded

	void pause();	// pause the running tlcs900h emulation
	void step();	// step to the next opcode
	void resume();	// continue where we left off
	void clear();   // clear the state machine (debugger acknowledges we are done)
	unsigned char state();	// what state is our debugger in?

	// Wrap get/set in non-debug version
	char * m_decodeList[0x200000];

private:
	char * bufPage;								// buf out a big page for instructions ( greatly increase speed )

	unsigned char * getCodePtr(unsigned long);	// Get the pointer to memory

	// Decode 0x80-0xF5
	int decodeXX(unsigned long, unsigned char);
	int decodeBytes(unsigned long, unsigned char, char **, unsigned char);

	// Give us 128 breakpoints to start with
	tlcs900hBreakpoint m_breakpointList[128];

	// Current debug state
	unsigned char m_debugState;
};

#endif