#pragma once

// Memory containers inside of NeoGPC
// 000000 -> 0000FF	CPU Internal RAM (Timers / DMA / RTC / IO / etc.)
// 004000 -> 006BFF	Work RAM
// 006C00 -> 006FFF	Bios work RAM
// 007000 -> 007FFF	Z80 Shared RAM
// 008000 -> 008FFF	Video registers
// 009000 -> 00BFFF	Video RAM
// 
// 200000 -> 3FFFFF	ROM (GAME CARTRIDGE)
// 800000 -> 9FFFFF	Extra ROM (for 32 Mbit games)
// FF0000 -> FFFFFF	BIOS

// Actual storage of memory
extern unsigned char memCPURAM[];
extern unsigned char memRAM[];
extern unsigned char memROM[];
extern unsigned char * mem32ROM;
extern unsigned char memBios[];

// Special flag for input state
extern unsigned char memInputState;

// Real bios load
extern bool memRealBios;

// Memory for NeoGPC
void memory_init();

// Get the appropriate memory
unsigned char * get_address(unsigned long addr);

// TLCS900h Memory Functions
unsigned char tlcsMemReadByte(unsigned long); // unsigned long address
unsigned short tlcsMemReadWord(unsigned long); // unsigned long address
unsigned long tlcsMemReadLong(unsigned long); // unsigned long address
void tlcsMemWriteByte(unsigned long, unsigned char); // address, char
void tlcsMemWriteWord(unsigned long, unsigned short); // address, short
void tlcsMemWriteLong(unsigned long, unsigned long); // address, long

// Z80 Memory Functions
unsigned char z80MemReadByte(unsigned long);
unsigned short z80MemReadWord(unsigned long);
void z80MemWriteByte(unsigned long, unsigned char);
void z80MemWriteWord(unsigned long, unsigned short);
void z80PortWriteByte(unsigned char, unsigned char);
unsigned char z80PortReadByte(unsigned char);