#include "memory.h"

// Flash memory support
#include "../core/flash.h"

// Sound support
#include "../core/sound.h"

// TLCS900h (NGPC CPU core)
#include "../cpu/tlcs900h.h"
#include "../cpu/z80.h"

// NeoPop Sound Engine
#include "../core/neopopsound.h"

// STL includes
#include <cstring>

//
// Memory Map Summary:
// -------------------
// 
// 000000 -> 0000FF	CPU Internal RAM (Timers / DMA / RTC / IO / etc.)
// 004000 -> 006BFF	Work RAM
// 006C00 -> 006FFF	Bios work RAM
// 007000 -> 007FFF	Z80 Shared RAM (Sound RAM)
// 008000 -> 008FFF	Video registers
// 009000 -> 00BFFF	Video RAM
// 
// 200000 -> 3FFFFF	ROM (GAME CARTRIDGE)
// 800000 -> 9FFFFF	Extra ROM (for 32 Mbit games)
// FF0000 -> FFFFFF	BIOS
//
unsigned char memCPURAM [0x0008A0];          // 0x000000-0x0000FF (CPU RAM) --- WHY DOES RACE THINK WE NEED MOAR?
unsigned char memRAM    [0x008000];             // 0x004000-0x00BFFF (RAM)
unsigned char memROM    [0x400000];             // 0x200000-0x3FFFFF (ROM)
unsigned char * mem32ROM = &memROM[0x200000];   // 0x800000-0x9FFFFF (32-BIT ROM)
unsigned char memBios   [0x010000];             // 0xFF0000-0xFFFFFF (BIOS)

unsigned char memInputState;

bool memRealBios;

// From RACE emulator

// Make more accurate!
const unsigned char ngpcpuram[256] = {
	// 0x00
	0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0xFF,
	// 0x10
	0x34, 0x3C, 0xFF, 0xFF, 0xFF, 0x3F, 0x00, 0x00, 0x3F, 0xFF, 0x2D, 0x01, 0xFF, 0xFF, 0x03, 0xB2,
	// 0x20
	0x80, 0x00, 0x01, 0x90, 0x03, 0xB0, 0x90, 0x62, 0x05, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x4C, 0x4C,
	// 0x30
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x20, 0xFF, 0x80, 0x7F,
	// 0x40
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	// 0x50
	0x00, 0x20, 0x69, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	// 0x60
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x03, 0x03, 0x02, 0x00, 0x00, 0x00,
	// 0x70 (0x70-0x7A: interrupt level settings)
	0x02, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	//
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned long ngpVectors[0x21] = {
	0x00FFF800, 0x00FFF000, 0x00FFF800, 0x00FFF801,	// 00, 04, 08, 0C
	0x00FFF80A, 0x00FFF813, 0x00FFF81C, 0x00FFF800,	// 10, 14, 18, 1C
	0x00FFF800, 0x00FFF800, 0x00FFF825, 0x00FFF82E, // 20, 24, 28, 2C
	0x00FFF837, 0x00FFF800, 0x00FFF800, 0x00FFF800,	// 30, 34, 38, 3C
	0x00FFF840, 0x00FFF849, 0x00FFF852, 0x00FFF85B,	// 40, 44, 48, 4C
	0x00FFF800, 0x00FFF800, 0x00FFF800, 0x00FFF800,	// 50, 54, 58, 5C
	0x00FFF864, 0x00FFF86D, 0x00FFF800, 0x00FFF800,	// 60, 64, 68, 6C
	0x00FFF800, 0x00FFF87F, 0x00FFF888, 0x00FFF891,	// 70, 74, 78, 7C
	0x00FFF89A										         // 80
};

const unsigned char ngpInterruptCode[] = {
	0x07,					      // RETI
	0xD1, 0xBA, 0x6F, 0x04,	// PUSHW (6FBA)	FFF801	SWI3
	0xD1, 0xB8, 0x6F, 0x04,	// PUSHW (6FB8)
	0x0E,					      // RET
	0xD1, 0xBE, 0x6F, 0x04,	// PUSHW (6FBE)   FFF80A	SWI4
	0xD1, 0xBC, 0x6F, 0x04,	// PUSHW (6FBC)
	0x0E,					      // RET
	0xD1, 0xC2, 0x6F, 0x04,	// PUSHW (6FC2)	FFF813	SWI5
	0xD1, 0xC0, 0x6F, 0x04,	// PUSHW (6FC0)
	0x0E,					      // RET
	0xD1, 0xC6, 0x6F, 0x04,	// PUSHW (6FC6)	FFF81C	SWI6
	0xD1, 0xC4, 0x6F, 0x04,	// PUSHW (6FC4)
	0x0E,					      // RET
	0xD1, 0xCA, 0x6F, 0x04,	// PUSHW (6FCA)	FFF825	RTC Alarm
	0xD1, 0xC8, 0x6F, 0x04,	// PUSHW (6FC8)
	0x0E,					      // RET
	0xD1, 0xCE, 0x6F, 0x04,	// PUSHW (6FCE)	FFF82E	VBlank
	0xD1, 0xCC, 0x6F, 0x04,	// PUSHW (6FCC)
	0x0E,					      // RET
	0xD1, 0xD2, 0x6F, 0x04,	// PUSHW (6FD2)	FFF837	interrupt from z80
	0xD1, 0xD0, 0x6F, 0x04,	// PUSHW (6FD0)
	0x0E,					      // RET
	0xD1, 0xD6, 0x6F, 0x04,	// PUSHW (6FD6)	FFF840	INTT0
	0xD1, 0xD4, 0x6F, 0x04,	// PUSHW (6FD4)
	0x0E,					      // RET
	0xD1, 0xDA, 0x6F, 0x04,	// PUSHW (6FDA)	FFF849	INTT1
	0xD1, 0xD8, 0x6F, 0x04,	// PUSHW (6FD8)
	0x0E,					      // RET
	0xD1, 0xDE, 0x6F, 0x04,	// PUSHW (6FDE)	FFF852	INTT2
	0xD1, 0xDC, 0x6F, 0x04,	// PUSHW (6FDC)
	0x0E,					      // RET
	0xD1, 0xE2, 0x6F, 0x04,	// PUSHW (6FE2)	FFF85B	INTT3 (interrupt to z80)
	0xD1, 0xE0, 0x6F, 0x04,	// PUSHW (6FE0)
	0x0E,					      // RET
	0xD1, 0xE6, 0x6F, 0x04,	// PUSHW (6FE6)	FFF864	Serial Receive
	0xD1, 0xE4, 0x6F, 0x04,	// PUSHW (6FE4)
	0x0E,					      // RET
	0xD1, 0xEA, 0x6F, 0x04,	// PUSHW (6FEA)	FFF86D	Serial Communication
	0xD1, 0xE8, 0x6F, 0x04,	// PUSHW (6FE8)
	0x0E,					      // RET
	0xD1, 0xEE, 0x6F, 0x04,	// PUSHW (6FEE)	FFF876	Reserved
	0xD1, 0xEC, 0x6F, 0x04,	// PUSHW (6FEC)
	0x0E,					      // RET
	0xD1, 0xF2, 0x6F, 0x04,	// PUSHW (6FF2)	FFF87F	End DMA Channel 0
	0xD1, 0xF0, 0x6F, 0x04,	// PUSHW (6FF0)
	0x0E,					      // RET
	0xD1, 0xF6, 0x6F, 0x04,	// PUSHW (6FF6)	FFF888	End DMA Channel 1
	0xD1, 0xF4, 0x6F, 0x04,	// PUSHW (6FF4)
	0x0E,					      // RET
	0xD1, 0xFA, 0x6F, 0x04,	// PUSHW (6FFA)	FFF891	End DMA Channel 2
	0xD1, 0xF8, 0x6F, 0x04,	// PUSHW (6FF8)
	0x0E,					      // RET
	0xD1, 0xFE, 0x6F, 0x04,	// PUSHW (6FFE)	FFF89A	End DMA Channel 3
	0xD1, 0xFC, 0x6F, 0x04,	// PUSHW (6FFC)
	0x0E,					      // RET
};

// Initialize the RACE bios
void race_bios()
{
	unsigned int i, x;

	// Fake BIOS From RACE emulator
	// Assume we are not loading a bios for now
	for(i = 0; i < 0x40; i++)
	{
		// Setup our fake jump table
		memBios[0xe000 + 0x40*i] = 0xc8;
		memBios[0xe001 + 0x40*i] = 0x1a;
		memBios[0xe002 + 0x40*i] = i;
		memBios[0xe003 + 0x40*i] = 0x0e;
		*((unsigned long *)(&memBios[0xfe00 + 4*i])) = (unsigned long)0x00ffe000 + 0x40 * i;
	}
   
	// setup SWI 1 code & vector
	x = 0xf000;
	memBios[x] = 0x17; x++; memBios[x] = 0x03; x++;		// ldf 3
	memBios[x] = 0x3C; x++;								      // push XIX
	memBios[x] = 0xC8; x++; memBios[x] = 0xCC; x++;		// and w,1F
	memBios[x] = 0x1F; x++;
	memBios[x] = 0xC8; x++; memBios[x] = 0x80; x++;		// add w,w
	memBios[x] = 0xC8; x++; memBios[x] = 0x80; x++;		// add w,w
	memBios[x] = 0x44; x++; memBios[x] = 0x00; x++;		// ld XIX,0x00FFFE00
	memBios[x] = 0xFE; x++; memBios[x] = 0xFF; x++;		//
	memBios[x] = 0x00; x++;								      //
	memBios[x] = 0xE3; x++; memBios[x] = 0x03; x++;		// ld XIX,(XIX+W)
	memBios[x] = 0xF0; x++; memBios[x] = 0xE1; x++;		//
	memBios[x] = 0x24; x++;								      //
	memBios[x] = 0xB4; x++; memBios[x] = 0xE8; x++;		// call XIX
	memBios[x] = 0x5C; x++;								      // pop XIX
	memBios[x] = 0x07; x++;								      // reti
	*((unsigned long *)(&memBios[0x00ff04])) = (unsigned long)0x00fff000;

	// setup interrupt codes
	for(i=0; i< sizeof(ngpInterruptCode); i++) {
		memBios[0x00f800+i] = ngpInterruptCode[i];
	}

	// setup interrupt vectors
	for(i=0; i< sizeof(ngpVectors)/4; i++) {
		*((unsigned long *)(&memBios[0x00ff00 + 4*i])) = ngpVectors[i];
	}
   
   // setup the CPU ram
   // interrupt priorities, timer settings, transfer settings, etc
   for(i=0; i<sizeof(ngpcpuram); i++)
   {
         memCPURAM[i] = ngpcpuram[i];
   }

   //koyote.bin handling (what is koyote.bin? a NGPC memory dump?)
   // memcpy(memRAM, koyote_bin, KOYOTE_BIN_SIZE/*12*1024*/);

   // setup interrupt vectors in RAM (this looks off)
   for(i=0; i<18; i++) {
	   *((unsigned long *)(&memRAM[0x2FB8+4*i])) = (unsigned long)0x00FFF800;
   }

   memRAM[0x2F80] = 0xFF; // Lots of battery power!
   memRAM[0x2F81] = 0x03;
   memRAM[0x2F95] = 0x10;
   memRAM[0x2F91] = 0x10; // Colour bios
   memRAM[0x2F84] = 0x40; // "Power On" startup
   memRAM[0x2F85] = 0x00; // No shutdown request
   memRAM[0x2F86] = 0x00; // No user answer (?)
   memRAM[0x2F87] = 0x01; // English

   memRAM[0x4000] = 0xC0;		   // Enable generation of VBlanks by default
   memRAM[0x4004] = 0xFF;	memRAM[0x4005] = 0xFF;
   memRAM[0x4006] = 0xC6;
   for(i=0; i<5; i++) { // 0x00070707 0x00070707 what is this purpose?
      memRAM[0x4101+4*i] = 0x07;	memRAM[0x4102+4*i] = 0x07;	memRAM[0x4103+4*i] = 0x07;
   }
   memRAM[0x4118] = 0x07; // ???
   // What is 0x63E0-0x63FF used for?
   memRAM[0x43E0] = memRAM[0x43F0] = 0xFF; memRAM[0x43E1] = memRAM[0x43F1] = 0x0F;
   memRAM[0x43E2] = memRAM[0x43F2] = 0xDD; memRAM[0x43E3] = memRAM[0x43F3] = 0x0D;
   memRAM[0x43E4] = memRAM[0x43F4] = 0xBB; memRAM[0x43E5] = memRAM[0x43F5] = 0x0B;
   memRAM[0x43E6] = memRAM[0x43F6] = 0x99; memRAM[0x43E7] = memRAM[0x43F7] = 0x09;
   memRAM[0x43E8] = memRAM[0x43F8] = 0x77; memRAM[0x43E9] = memRAM[0x43F9] = 0x07;
   memRAM[0x43EA] = memRAM[0x43FA] = 0x44; memRAM[0x43EB] = memRAM[0x43FB] = 0x04;
   memRAM[0x43EC] = memRAM[0x43FC] = 0x33; memRAM[0x43ED] = memRAM[0x43FD] = 0x03;
   memRAM[0x43EE] = memRAM[0x43FE] = 0x00; memRAM[0x43EF] = memRAM[0x43FF] = 0x00;
}

// Initialize the Memory
void memory_init()
{
	unsigned int i;

   // Zero our input state
   memInputState = 0;

   // Zero out our RAM
   memset(memCPURAM, 0, sizeof(memCPURAM));
   memset(memRAM, 0, sizeof(memRAM));

   // Zero out bios ROM
   memset(memBios, 0, sizeof(memBios));

   // Assume false
   memRealBios = false;

   // Try to load the bios (ngpbios.bin) and set PC to our bios, otherwise do this
   FILE * biosFile = fopen("ngpbios.bin", "rb");
   if ( biosFile != NULL )
   {
	   long lSize;
	   fseek(biosFile, 0, SEEK_END);
	   lSize = ftell(biosFile);
	   rewind(biosFile);
	   // Bios must be 64kb, if so read it into our bios
	   if ( lSize == 0x10000 )
	   {
		   fread(memBios, 1, lSize, biosFile);
		   memRealBios = true;
		   fclose(biosFile);
	   }
	   else
	   {
		   race_bios();
	   }
   }
   else
   {
	   race_bios();
   }
}


unsigned char * get_address(unsigned long addr)
{
	addr&= 0x00FFFFFF;
	if (addr<0x00200000)
	{
		if (addr<0x000008a0) // why is this 0x8A0 and not 0xFF???
		{
		    //if(((unsigned long)&cpuram[addr] >> 24) == 0xFF)
            //    dbg_print("1) addr=0x%X returning=0x%X\n", addr, &cpuram[addr]);
			return &memCPURAM[addr];//cpuram[addr];
		}
		if (addr>0x00003fff && addr<0x00018000)
        {
            //if((unsigned long)&mainram[addr-0x00004000] >> 24 == 0xFF)
            //    dbg_print("2) addr=0x%X returning=0x%X\n", addr, &mainram[addr-0x00004000]);

            switch (addr)  //Thanks Koyote
            {
                case 0x6F80:
                    memRAM[addr-0x00004000] = 0xFF;
                    break;
                case 0x6F80+1:
                    memRAM[addr-0x00004000] = 0x03;
                    break;
                case 0x6F85:
                   memRAM[addr-0x00004000] = 0x00; //??
                    break;
                case 0x6F82:
                    memRAM[addr-0x00004000] = memInputState;
                    break;
                case 0x6DA2:
                    memRAM[addr-0x00004000] = 0x80;
                    break;
            }
            return &memRAM[addr-0x00004000];
        }
	}
	else
	{
		if (addr<0x00400000)
		{
            //if((unsigned long)&mainrom[addr-0x00200000] >> 24 == 0xFF)
            //    dbg_print("3) addr=0x%X returning=0x%X\n", addr, &mainrom[addr-0x00200000]);
            return &memROM[(addr-0x00200000)];
		}
        if(addr<0x00800000) //Flavor added
		{
		    //dbg_print("3.5) addr=0x%X returning=0\n", addr);
            return 0;
		}
		if (addr<0x00A00000)
		{
            //if((unsigned long)&mainrom[addr-0x00800000+0x00200000] >> 24 == 0xFF)
            //    dbg_print("4) addr=0x%X returning=0x%X\n", addr, &mainrom[addr-0x00800000+0x00200000]);
            return &mem32ROM[(addr-0x00800000)];
		}
		if(addr<0x00FF0000) //Flavor added
		{
		    //dbg_print("4.5) addr=0x%X returning=0\n", addr);
            return 0;
		}

        //if((unsigned long)&cpurom[addr-0x00ff0000] >> 24 == 0xFF)
        //   dbg_print("5) addr=0x%X returning=0x%X\n", addr, &cpurom[addr-0x00ff0000]);

        return &memBios[addr-0x00ff0000];
	}

	//dbg_print("6) addr=0x%X returning=0\n", addr);
	return 0;  //Flavor ERROR
}

// TLCS900h Memory Functions
unsigned char tlcsMemReadByte(unsigned long addr)
{
   addr &= 0x00FFFFFF; // only work between 0x00000000-0x00FFFFFFFF

   // Are we currently reading from Flash memory?
   if(g_flashCommand == COMMAND_INFO_READ)
        return flashReadInfo(addr);

   // Check to see which section of memory we're trying to read from (RAM/ROM)
   if (addr < 0x00200000) { // RAM!
      if (addr < 0x000008A0) { // why 0x8A0??? why not 0xFF
         if(addr == 0xBC)      // special command to advance Z80 sound chip
         {
            ngpSoundExecute(); // make a sound
         }
         return memCPURAM[addr]; // flag if we try to read past 0xFF
      }
      // RAM only goes from 0x4000 to 0xBFFF
      else if (addr > 0x00003FFF && addr < 0x00018000)
      {
         switch (addr)  //Thanks Koyote (RACE!)
         {
		 //case 0x4000:
			// Noise Channel & Right Volume
		//	 return noiseChip.LastRegister; // ??? how do we do this?
			//return memRAM[addr-0x00004000];
		//	break;
		 //case 0x4001:
			// Tone Channel & Left Volume
		//	 return toneChip.LastRegister; // ??? how do we do this?
			 //return memRAM[addr-0x00004000];
		//	break;
		 case 0x6DA2:            // Purpose?
            return 0x80;
            break;
         case 0x6F80:            // Battery Power (0xFF = Lots!)
            return 0xFF;
            break;
         case 0x6F81:          // Battery Power (0x3FF = Lots!)
            return 0x03;
            break;
         case 0x6F82:
            return memInputState; // get the input state
            break;
		 //case 0x6F83:			// Boot something?
		//	return 0x00;
		//	break;
		 //case 0x6F84:			// User Boot
		//	 return 1<<6;		// Bit 0-4 = 0
								// Bit 5 = "Resume" startup: 1 = yes, 0 = no
								// Bit 6 = "Power On" startup: 1 = yes, 0 = no
								// Bit 7 = "Alarm" startup: 1 = yes, 0 = no
		//	 break;
		 case 0x6F85:            // Shutdown Request?
            return memRAM[addr-0x00004000];
            break;
		// case 0x6F86:			// User Answer
		//	 return 0x00;
		//	break;
		// case 0x6F87:			// Language?
		//	return 0x01;		// Force English for now
		//	break;
		 case 0x6F88:			// CPU clock
			return memRAM[addr-0x00004000];
			break;
		 //case 0x6F95:			// Reset RTC???
		//	return 0x00;		// breakpoint here
		//	break;
		 //case 0x8000:
		//	 return memCPURAM[0xBC];
		//	break;
         default:
            return memRAM[addr-0x00004000];
         }
         if ( addr < 0xC000 ) // What could we get between 0xC000-0x18000?
         {
            return memRAM[addr-0x00004000];
         }
         else // no man's land 0x0000C000-0x20000000
         {
            // Report this violation immediatrah!!!
            return 0xFF;
         }
      }
   }
	else // ROM!
	{
		if (addr<0x00400000) // standard ROM read (0x00200000-0x003FFFFF)
		{
			if ( g_flashCommand == COMMAND_INFO_READ )
				return flashReadInfo(addr);
            return memROM[(addr-0x00200000)];
		}
		if (addr<0x00800000) // no-man's land (0x00400000-0x007FFFFF)
            return 0xFF;
		if (addr<0x00a00000) // 32-bit ROM read (0x00800000-0x009FFFFF)
		{
			if ( g_flashCommand == COMMAND_INFO_READ )
				return flashReadInfo(addr);
            return mem32ROM[(addr-(0x00800000))]; //-0x00200000))]; // why an extra 0x00200000?
		}
		if (addr<0x00ff0000) // mo-nam's dan (0x00A00000-0x00FEFFFF)
            return 0xFF;
		return memBios[addr-0x00ff0000]; // assume we're making a bios read
	}

   // Read something we do not accomodate for
   return 0xFF;
}

unsigned short tlcsMemReadWord(unsigned long addr)
{
   return tlcsMemReadByte(addr) | (tlcsMemReadByte(addr+1) << 8);
}

unsigned long tlcsMemReadLong(unsigned long addr)
{
   return tlcsMemReadByte(addr) | (tlcsMemReadByte(addr+1) << 8)
      | (tlcsMemReadByte(addr+2) << 16) | (tlcsMemReadByte(addr+3) << 24);
}

void tlcsMemWriteByte(unsigned long addr, unsigned char data)
{
   addr &= 0x00FFFFFF;
   if (addr<0x000008a0)
   {
      switch(addr) {
      //case 0x80:	// CPU speed
      //    break;
      case 0xA0:	// L CH Sound Source Control Register
         if (memCPURAM[0xB8] == 0x55 && memCPURAM[0xB9] == 0xAA)
            Write_SoundChipNoise(data);//Flavor SN76496Write(0, data);
         break;
      case 0xA1:	// R CH Sound Source Control Register
         if (memCPURAM[0xB8] == 0x55 && memCPURAM[0xB9] == 0xAA)
            Write_SoundChipTone(data); //Flavor SN76496Write(0, data);
         break;
      case 0xA2:	// L CH DAC Control Register
         ngpSoundExecute();
         if (memCPURAM[0xB8] == 0xAA)
            dac_writeL(data); //Flavor DAC_data_w(0,data);
         break;
      case 0xA3:	// R CH DAC Control Register
		  ngpSoundExecute();
		if (memCPURAM[0xB8] == 0xAA)
			 dac_writeR(data);//Flavor DAC_data_w(1,data);
		break;
      case 0xB8:	// Z80 Reset
         //				if (data == 0x55)	DAC_data_w(0,0);
      case 0xB9:	// Sourd Source Reset Control Register
         switch(data) {
         case 0x55:
            ngpSoundStart();
            break;
         case 0xAA:
            ngpSoundExecute();
            ngpSoundOff();
            break;
         }
         break;
      case 0xBA:
         ngpSoundExecute();
         z80Interrupt(Z80NMI);
         break;
      }
      // 0x8a0 is outside of our CPU ram range
      memCPURAM[addr] = data;
      return;
   }
   else if (addr>0x00003fff && addr<0x00018000)
   {
      if (addr == 0x87E2 && memRAM[0x47F0] != 0xAA)
         return;		// disallow writes to GEMODE

      /*if((addr >= 0x8800 && addr <= 0x88FF)
      || (addr >= 0x8C00 && addr <= 0x8FFF)
      || (addr >= 0xA000 && addr <= 0xBFFF)
      || addr == 0x00008020
      || addr == 0x00008021)
      {
      spritesDirty = true;
      }*/
      if ( addr < 0x00BFFF ) // why would anyone write past 0xBFFF?
	  {
		  switch (addr)  //Thanks Koyote (RACE!)
         {
		  case 0x6F88:	// CPU speed ( bits 0-2 - Clock Speed )
			  switch(data&5)
			  {
			  case 1:
				  clockrate = CLOCK_3072;
				break;
			  case 2:
				  clockrate = CLOCK_1536;
			 break;
			  case 3:
				  clockrate = CLOCK_768;
			break;
			  case 4:
				  clockrate = CLOCK_384;
			break;
			  case 0:
			  default:
				  clockrate = CLOCK_6144;
				  break;
			  }
		  break;
		  default:
			  break;
         }
		  memRAM[addr-0x00004000] = data;
	  }
      return;
   }
   else if (addr >= 0x00200000 && addr < 0x00400000)
      flashChipWrite(addr, data);
   else if (addr >= 0x00800000 && addr < 0x00A00000)
      flashChipWrite(addr, data);
}

void tlcsMemWriteWord(unsigned long addr, unsigned short data)
{
   // Will this have side effects?
   tlcsMemWriteByte(addr, data & 0xFF);   // 0x00XX
   tlcsMemWriteByte(addr+1, data >> 8);   // 0xXX00
}

void tlcsMemWriteLong(unsigned long addr, unsigned long data)
{
   // Will this have side effects if it writes somewhere like sound?
   tlcsMemWriteByte(addr,   (data) & 0xFF);        // 0x000000XX
   tlcsMemWriteByte(addr+1, (data >> 8) & 0xFF);   // 0x0000XX00
   tlcsMemWriteByte(addr+2, (data >> 16) & 0xFF);  // 0x00XX0000
   tlcsMemWriteByte(addr+3, (data >> 24) & 0xFF);  // 0xXX000000
}

// Z80 Memory Functions
unsigned char z80MemReadByte(unsigned long addr)
{
	if (addr < 0x1000) { //0x4000) { // anything above 0x1000 will put us out of the Z80
		return memRAM[0x3000 + addr];
	}
	switch(addr) {
	case 0x4000:	// sound chip read (why does this return nothing?)
		break;
	case 0x4001:
		break;		// sound chip read second byte? (why do we want to do this?)
	case 0x8000:
        return memCPURAM[0xBC];
	case 0xC000:
		break;
	};
	return 0x00; // this is not valid, return nothing from our Z80
}

unsigned short z80MemReadWord(unsigned long addr)
{
   return (z80MemReadByte(addr+1) << 8) | z80MemReadByte(addr);
}

void z80MemWriteByte(unsigned long addr, unsigned char data)
{
   if (addr < 0x4000) {
      memRAM[0x3000 + addr] = data;
      return;
   }

   switch (addr)
   {
   case 0x4000:
      Write_SoundChipNoise(data);//Flavor SN76496Write(0, data);
      return;
   case 0x4001:
      Write_SoundChipTone(data);//Flavor SN76496Write(0, data);
      return;
   case 0x8000: // Trigger a data write to our CPU
      memCPURAM[0xBC] = data;
   return;
   case 0xC000: // cause an INT 3 on the TLCS900h
      tlcs_interrupt_wrapper(0x03);
      return;
   }
}

void z80MemWriteWord(unsigned long addr, unsigned short data)
{
   if (addr < 0x4000) {
		memRAM[0x3000 + addr] =   data&0xFF; // 0x00XX
		memRAM[0x3000 + addr+1] = data>>8;   // 0xXX00
		return;
	}

	switch (addr)
	{
	case 0x4000:
		Write_SoundChipNoise(data&0xFF);//Flavor SN76496Write(0, data);
		Write_SoundChipNoise(data>>8);//Flavor SN76496Write(0, data);
		return;
	case 0x4001:
		Write_SoundChipTone(data&0xFF);//Flavor SN76496Write(0, data);
		Write_SoundChipTone(data>>8);//Flavor SN76496Write(0, data);
		return;
	case 0x8000: // write to our CPU offset twice (feels redundant, should it be 0xBD?)
		memCPURAM[0xBC] = data&0xFF;
		memCPURAM[0xBC] = data>>8;
		return;
	case 0xC000:
		tlcs_interrupt_wrapper(0x03); // two interrupts
		tlcs_interrupt_wrapper(0x03);
		return;
	}
}

void z80PortWriteByte(unsigned char p, unsigned char data)
{
   // Acknowledge a port write, any port works ??
}

unsigned char z80PortReadByte(unsigned char)
{
   // RACE clearly did not implement this
   return 0xFF;
}
