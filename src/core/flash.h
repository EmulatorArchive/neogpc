#pragma once

// NeoGPC - Flash memory

#define NO_COMMAND              0x00
#define COMMAND_BYTE_PROGRAM    0xA0
#define COMMAND_BLOCK_ERASE     0x30
#define COMMAND_CHIP_ERASE      0x10
#define COMMAND_INFO_READ       0x90

extern unsigned char g_flashCommand;
extern char sg_romName[];

unsigned char flashReadInfo(unsigned long addr);
void flashChipWrite(unsigned long addr, unsigned char data);
void vectFlashWrite(unsigned char chip, unsigned int to, unsigned char *fromAddr, unsigned int numBytes);
void vectFlashErase(unsigned char chip, unsigned char blockNum);
void vectFlashChipErase(unsigned char chip);
void setFlashSize(unsigned long romSize, char * fileName);
void flashShutdown();

#define SAVEGAME_DIR "Battery/"
