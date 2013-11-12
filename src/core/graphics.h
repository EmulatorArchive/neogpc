#pragma once

// Graphics core for NeoGPC
//  - Cthulhu32 Oct 21 2012

bool graphics_init();
void graphics_blit();

extern unsigned short palettes[16*4+16*4+16*4]; // placeholder for the converted palette
extern int    totalpalette[32*32*32];
#define NGPC_TO_SDL16(col) totalpalette[col]

extern unsigned char *scanlineY;

//actual NGPC
#define NGPC_SIZEX 160
#define NGPC_SIZEY 152
extern unsigned short drawBuffer[NGPC_SIZEX*NGPC_SIZEY];

void graphicsBlit(unsigned char render);
