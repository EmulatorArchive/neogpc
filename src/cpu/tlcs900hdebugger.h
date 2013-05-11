#pragma once

// Include the NeoGPC core
#include "..\core\neogpc.h"

// Do not include code if we are not using the debugger
#ifdef NEOGPC_DEBUGGER

// TLCS 900h Breakpoint
typedef struct _tlcs900hBreakpoint {
	unsigned int address;
	bool active;
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

// Class for the TLCS900h Debugger
class tlcs900hdebugger
{
public:
	tlcs900hdebugger(void);
	~tlcs900hdebugger(void);

	tlcs900hBreakpoint * getBreakpointList();			// Return all breakpoints
	int setBreakpoint(unsigned int);		// Set a breakpoint at X location
	void removeBreakpoint(int);	// Disable breakpoint (index)
	void enableBreakpoint(int);	// Enable breakpoint (index)
	void disableBreakpoint(int); // Disable breakpoint (index)
	void clearBreakpoints();	// clear all the breakpoints

	void pause();	// pause the running tlcs900h emulation
	void step();	// step to the next opcode
	void resume();	// continue where we left off
	void clear();   // clear the state machine (debugger acknowledges we are done)
	unsigned char state();	// what state is our debugger in?

private:
	// Give us 128 breakpoints to start with
	tlcs900hBreakpoint m_breakpointList[128];

	// Current debug state
	unsigned char m_debugState;
};

#endif