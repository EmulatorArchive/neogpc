//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

// tlcs900h.h: interface for the tlcs900h class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TLCS900H_H__41E77E01_8224_11D3_8644_00A0241D2A65__INCLUDED_)
#define AFX_TLCS900H_H__41E77E01_8224_11D3_8644_00A0241D2A65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

extern unsigned char F2;
extern unsigned long
  gen_regsXWA0, gen_regsXBC0, gen_regsXDE0, gen_regsXHL0,
  gen_regsXWA1, gen_regsXBC1, gen_regsXDE1, gen_regsXHL1,
  gen_regsXWA2, gen_regsXBC2, gen_regsXDE2, gen_regsXHL2,
  gen_regsXWA3, gen_regsXBC3, gen_regsXDE3, gen_regsXHL3,
  gen_regsXIX,  gen_regsXIY,  gen_regsXIZ,  gen_regsXSP,
  gen_regsXSSP, gen_regsXNSP;
extern int timer0, timer1, timer2, timer3;
extern unsigned char ldcRegs[64];

// Clock rates
#define CLOCK_6144 6144000
#define CLOCK_3072 3072000
#define CLOCK_1536 1536000
#define CLOCK_768 768000
#define CLOCK_384 384000
extern unsigned int clockrate;

#define XWA0	0
#define XBC0	1
#define XDE0	2
#define XHL0	3
#define XWA1	4
#define XBC1	5
#define XDE1	6
#define XHL1	7
#define XWA2	8
#define XBC2	9
#define XDE2	10
#define XHL2	11
#define XWA3	12
#define XBC3	13
#define XDE3	14
#define XHL3	15
#define XIX		16
#define XIY		17
#define XIZ		18
#define XSP		19
#define PC		20
#define SR		21
#define XSSP	22
#define XNSP	23
// initialize registers, etc..
void tlcs_init();
void tlcs_reinit();
// perform one cpu step
int tlcs_step();
// output the current contents of the registers to a file
//void tlcs_print(FILE *output);
// execute interrupt
void tlcs_interrupt_wrapper(int irq);
// check PC
//int check_pc(unsigned long addr);
// input from TI0
void tlcsTI0();
//
//void tlcs_test();
//
//void setErrorLog(FILE *errorlog, FILE *outputram);
//void closeLog();
//void ngpc_run();

void tlcs_execute(int cycles);

// For defines in NeoGPC, take these out for a non-NeoGPC emulator
#include "../core/neogpc.h"

// Do we want to use the debugger?
#ifdef NEOGPC_DEBUGGER

#include "../cpu/tlcs900hdebugger.h"

#include "../cpu/z80.h"

// Create our global tlcs900h debugger
extern tlcs900hdebugger g_tlcs900hDebugger;

extern unsigned long gen_regsXWA0, gen_regsXBC0, gen_regsXDE0, gen_regsXHL0, gen_regsXWA1,
gen_regsXBC1, gen_regsXDE1, gen_regsXHL1, gen_regsXWA2, gen_regsXBC2,
gen_regsXDE2, gen_regsXHL2, gen_regsXWA3, gen_regsXBC3, gen_regsXDE3,
gen_regsXHL3, gen_regsXIX, gen_regsXIY, gen_regsXIZ, gen_regsXSP,
gen_regsXSSP, gen_regsXNSP;

extern unsigned long gen_regsPC, gen_regsSR;

#endif

#endif // !defined(AFX_TLCS900H_H__41E77E01_8224_11D3_8644_00A0241D2A65__INCLUDED_)
