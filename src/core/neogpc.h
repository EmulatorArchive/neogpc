#pragma once

// Uncomment to enable NeoGPC file logging
//#define NEOGPC_LOG_ENABLED

// Version of our NeoGPC build
#define NEOGPC_VERSION "1.1"

// Uncomment to enable NeoGPC TLCS900h Debugger
#define NEOGPC_DEBUGGER
#ifdef NEOGPC_DEBUGGER

	#include "../cpu/tlcs900hdebugger.h"	
	#include <map>

	class tlcs900hdebugger; // forward declare so we know this is accessible
	extern tlcs900hdebugger g_tlcs900hDebugger;

	int neogpc_setbreakpoint(unsigned int, unsigned char);
	char * neogpc_getbreakpointBuffer(unsigned int);
	void neogpc_deletebreakpoint(int);
	void neogpc_stepindebugger();
	void neogpc_stepoverdebugger();
	void neogpc_pausedebugger();
	void neogpc_resumedebugger();
	void neogpc_cleardebugger();

	void neogpc_disassemble();
	char * neogpc_asmprint(unsigned int);
	int neogpc_asminc(unsigned int);
#endif

//the number of frames we want to draw to the host's screen every second
#define HOST_FPS 60

// NeoGPC main emulate function
void neogpc_init();
bool neogpc_loadrom(char*,int,char*);
void neogpc_emulate(unsigned int);
void neogpc_shutdown();

//RomHeader (from NeoPop)
typedef struct _RomHeader
{
	char	license[28];		         // 0x00 - 0x1B
	unsigned int	startPC;			   // 0x1C - 0x1F
	unsigned short	catalog;			   // 0x20 - 0x21
	unsigned char	subCatalog;			// 0x22
	unsigned char	mode;				   // 0x23
	char	name[12];			         // 0x24 - 0x2F
	unsigned int	reserved1;			// 0x30 - 0x33
	unsigned int	reserved2;			// 0x34 - 0x37
	unsigned int	reserved3;			// 0x38 - 0x3B
	unsigned int	reserved4;			// 0x3C - 0x3F
}
RomHeader;

// Loaded rom
extern RomHeader * g_currentRom;
