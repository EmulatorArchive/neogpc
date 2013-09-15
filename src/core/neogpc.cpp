#include "neogpc.h"
#include "memory.h"

// For local file logging
#include "log.h"

// Use STL when we can
#include <cstring>

// CPUs (TLCS900h and Z80)
#include "..\cpu\tlcs900h.h"
#include "..\cpu\z80.h"

// Core includes
#include "..\core\graphics.h"
#include "..\core\sound.h"
#include "..\core\neopopsound.h"
#include "..\core\flash.h"

// Global headers for a true NGPC rom
char * romCopyright = "COPYRIGHT BY SNK CORPORATION";
char * romLicensed =  " LICENSED BY SNK CORPORATION";

RomHeader * g_currentRom;

// Init everything that goes into NeoGPC
void neogpc_init()
{
   // This has to go after the ROM has been loaded..

   // Init the system memory
   memory_init();

   // Init the system graphics
   graphics_init();

   // Init the TLCS900h core
   tlcs_init();

   // Init the Z80 cpu
   z80Init();

   // RACE rom hacks
   // if neogeo pocket color rom, act if we are a neogeo pocket color
   tlcsMemWriteByte(0x6F91,tlcsMemReadByte(0x00200023));
   // pretend we're running in English mode
   tlcsMemWriteByte(0x00006F87,0x01);
   // kludges & fixes
   switch (tlcsMemReadWord(0x00200020))
   {
      case 0x0059:	// Sonic
      case 0x0061:	// Metal SLug 2nd
         *get_address(0x0020001F) = 0xFF;
         break;
   }

   // Initialize the sound system
   // sound_system_init();

   ngpSoundOff();
   //Flavor sound_start();
}

// Load the NGPC rom, make sure its valid
bool neogpc_loadrom(char * rom, int romLen, char * romName)
{
   // Assume the rom header is valid
   RomHeader * romHeader = (RomHeader *)(&(rom[0]));

   // Detect Licensed by SNK (more common)
   if ( strncmp(romHeader->license, romLicensed, 28) != 0 )
   {
      // Detect Copyright by SNK (less common)
      if ( strncmp(romHeader->license, romCopyright, 28) != 0)
      {
         return false;
      }
   }

   LOG("File looks okay! License: %s Name : %s\r\n", romHeader->license, romHeader->name);

   // Set our pointer to this
   g_currentRom = romHeader;
   
   // Copy the file to our ROM bank (32-bit rom bank?)
   //if ( romLen < 0x ) // 32kb?
   memcpy(memROM, rom, romLen);

   // Setup our flash rom size
   setFlashSize(romLen, romName);

   return true;
}

// Step according to our frameskip value
void neogpc_emulate(unsigned int frames)
{
   // Emulate X number of frames according to our host FPS
   tlcs_execute((6*1024*1000) / HOST_FPS); // 6144000 = 6.144 MHz processor
}

// Shut everything down
void neogpc_shutdown()
{
	flashShutdown();		// Save the flash memory
}

// TLCS900h Debugger
// Uncomment to enable NeoGPC TLCS900h Debugger
#ifdef NEOGPC_DEBUGGER

// set the breakpoint
int neogpc_setbreakpoint(unsigned int address)
{
	return g_tlcs900hDebugger.setBreakpoint(address);
}

void neogpc_setbreakpointName(unsigned int idx, const char * name)
{
	g_tlcs900hDebugger.setBreakpointName(idx, name);
}

char * neogpc_getbreakpointBuffer(unsigned int idx)
{
	return g_tlcs900hDebugger.getBreakpointName(idx);
}

// remove the breakpoint
void neogpc_deletebreakpoint(int index)
{
	g_tlcs900hDebugger.removeBreakpoint(index);
}

// Step the debugger
void neogpc_stepdebugger()
{
	g_tlcs900hDebugger.step();
}

// Pause the debugger
void neogpc_pausedebugger()
{
	g_tlcs900hDebugger.pause();
}

// Resume the debugger
void neogpc_resumedebugger()
{
	g_tlcs900hDebugger.resume();
}

// Clear the debugger and resume the rom
void neogpc_cleardebugger()
{
	g_tlcs900hDebugger.clearBreakpoints();
	g_tlcs900hDebugger.resume();
}

// Disassemble the ROM
void neogpc_disassemble()
{
	unsigned long addr = g_currentRom->startPC; //0x00200040; // 0x000000 - 0xFFFFFF
	do
	{
		addr += g_tlcs900hDebugger.decodeTlcs900h(addr);
	} while ( addr < 0x00400000 ); // detect 32-bit rom size
}

// Get a disassembled line
char * neogpc_asmprint(unsigned int addr)
{
	return g_tlcs900hDebugger.getBufString(addr);
}

// Get the increment debug
int neogpc_asminc(unsigned int addr)
{
	return g_tlcs900hDebugger.getInc(addr);
}

#endif
