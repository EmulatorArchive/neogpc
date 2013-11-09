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
	DEBUGGER_BREAK,
	DEBUGGER_RESUME,
	DEBUGGER_STEPIN,
	DEBUGGER_STEPOVER,
	DEBUGGER_OVER,
	DEBUGGER_INTO
};


#define OPS_LEN 80
#define MEM_LEN 80

// TLCS900h struct
struct tlcs900d {
  unsigned int addr;
  //unsigned char *buffer;
  //unsigned int pos;
  //unsigned int base;
  int len;
  int opt;        // Output type..
  char *opf;
  char ops[OPS_LEN];
  //FILE *fh;
  //int lines;
  //int space;
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
	void removeBreakpoint(int);	// Disable breakpoint (index)
	void enableBreakpoint(int);	// Enable breakpoint (index)
	void disableBreakpoint(int); // Disable breakpoint (index)
	void clearBreakpoints();	// clear all the breakpoints

	char * getBufString(unsigned int); // Get the buffer string
	int getInc(unsigned int); // Get the incrmeent we recorded during debugging

	unsigned int decodeTlcs900h(const unsigned int);	// decode the current PC, returning bytes eaten
	char * lastInstruction();		// get the last instruction that was decoded

	void breakp();			// trigger breakpoint on the running tlcs900h emulation, 
	void pause();			// pause the running tlcs900h emulation
	void stepin();			// step to the next opcode
	void stepover();		// step over the next opcode (incase its a call)
	void resume();			// continue where we left off
	void clear();			// clear the state machine (debugger acknowledges we are done)
	unsigned char state();	// what state is our debugger in?

private:
	char * bufPage;								// buf out a big page for instructions ( greatly increase speed )
	unsigned int bufInc[0x200000];				// how much does each buf increment by?

	unsigned char * getCodePtr(unsigned long);	// Get the pointer to memory

	// Decode opcodes that do not require a decode
	int decodeFixed( struct tlcs900d * );
	int decode_B0_mem( struct tlcs900d * );
	int decode_xx( struct tlcs900d * );
	int decode_zz_r( struct tlcs900d * );
	int decode_zz_R( struct tlcs900d * );
	int decode_zz_mem( struct tlcs900d * );

	// Give us 128 breakpoints to start with
	tlcs900hBreakpoint m_breakpointList[128];

	// Current debug state
	unsigned char m_debugState;

	struct tlcs900d m_dd;

};

#endif