#include "tlcs900hdebugger.h"

// Do not include code if we are not using the debugger
#ifdef NEOGPC_DEBUGGER

#include <string.h>
#include <stdio.h>
#include <algorithm> // sorting

// ROM, RAM, Bios
#include "../core/memory.h"

enum opcodes {
	LD=0,	LDW,  PUSH,	PUSHW,  POP,	POPW,   LDA,	  LDAR,
	EX,		MIRR, LDI,  LDIW,	  LDIR, LDIRW,	LDD,	  LDDW,
  LDDR, LDDRW,CPI,	CPIR,	  CPD,	CPDR, 	ADD,	  ADDW,
  ADC,	ADCW, SUB,	SUBW,   SBC,	SBCW,   CP,     CPW,
  INC,  INCW,	DEC,  DECW,	  NEG,  EXTZ,	  EXTS,	  DAA,
	PAA,	MUL,	MULS,	DIV,	  DIVS,	MULA,	  MINC1,  MINC2,
  MINC4,MDEC1,MDEC2,MDEC4,	AND,	ANDW,   OR,     ORW,
  XOR,  XORW,	CPL,  LDCF,	  STCF,	ANDCF,	ORCF,	  XORCF,
  RCF,	SCF,	CCF,	ZCF,	  BIT,	RES,	  SET,	  CHG,
  TSET,	BS1F, BS1B,	NOP,	  EI,		DI,		  SWI,	  HALT,
  LDC,	LDX,  LINK,	UNLK,	  LDF,  INCF,   DECF,	  SCC,
  RLC,	RLCW, RRC, RRCW, RL, RLW,  RR, RRW,
  SLA, SLAW, SRA, SRAW, SLL, SLLW,  SRL, SRLW,
  RLD,	RRD,  JP,	  JR,	    JRL,  CALL,   CALR,	  DJNZ,
  RET,	RETD, RETI,
  INVALID
};

enum maddressingmodes {
	ARI_XWA=0,ARI_XBC,ARI_XDE,ARI_XHL,ARI_XIX,ARI_XIY,ARI_XIZ,ARI_XSP,
	ARID_XWA,ARID_XBC,ARID_XDE,ARID_XHL,ARID_XIX,ARID_XIY,ARID_XIZ,ARID_XSP,
	ABS_B,ABS_W,ABS_L,
	ARI,
	ARI_PD,ARI_PI
};

enum output_types {
  OPT_1_0_0, OPT_1_1_0, OPT_1_1_1, OPT_1_1_2, OPT_1_2_0, OPT_1_3_0,
  OPT_2_1_2, OPT_2_0_0, OPT_1_n_1, OPT_1_n_1_1, OPT_1_n_1_2,
  OPT_1_n_2, OPT_1_n_1_4, OPT_1_4_0, OPT_1_1_1_1_1_1
};

char *opcode_names[] = {
  "LD",   "LDW",  "PUSH",	"PUSHW",  "POP",	  "POPW",   "LDA",	  "LDAR",
	"EX",   "MIRR", "LDI",	"LDIW",   "LDIR",	  "LDIRW",  "LDD",    "LDDW",
  "LDDR", "LDDRW","CPI",	"CPIR",	  "CPD",	  "CPDR", 	"ADD",	  "ADDW",
  "ADC",	"ADCW", "SUB",  "SUBW", 	"SBC",    "SBCW",   "CP",     "CPW",
  "INC",  "INCW", "DEC",  "DECW", 	"NEG",  	"EXTZ",	  "EXTS",	  "DAA",
  "PAA",  "MUL",  "MULS", "DIV",    "DIVS",   "MULA",   "MINC1",  "MINC2",
  "MINC4","MDEC1","MDEC2","MDEC4",  "AND",	  "ANDW",   "OR",     "ORW",
  "XOR",  "XORW", "CPL",  "LDCF",   "STCF",   "ANDCF",  "ORCF",	  "XORCF",
  "RCF",  "SCF",  "CCF",  "ZCF",    "BIT",    "RES",    "SET",    "CHG",
  "TSET", "BS1F", "BS1B", "NOP",    "EI",     "DI",     "SWI",    "HALT",
  "LDC",  "LDX",  "LINK",	"UNLK",	  "LDF",	  "INCF",   "DECF",   "SCC",
  "RLC",  "RLCW", "RRC",  "RRCW", "RL", "RLW",  "RR", "RRW",
  "SLA", "SLAW", "SRA", "SRAW", "SLL", "SLLW", "SRL", "SRLW",
  "RLD",	"RRD",  "JP",   "JR",	    "JRL",	  "CALL",   "CALR",	  "DJNZ",
  "RET",	"RETD",   "RETI",
  "INVALID"
};

char *cr_names[] = {
    "DMAS0","?","?","?",
    "DMAS1","?","?","?",
    "DMAS2","?","?","?",
    "DMAS3","?","?","?",
    "DMAD0","?","?","?",
    "DMAD1","?","?","?",
    "DMAD2","?","?","?",
    "DMAD3","?","?","?",
    "DMAC0","?","DMAM0","?",
    "DMAC1","?","DMAM1","?",
    "DMAC2","?","DMAM2","?",
    "DMAC3","?","DMAM3","?",
    "INTNEST","?","?","?"
};

char *cc_names[] = {
    "F",    "LT",
    "LE",   "ULE",
    "PE",   "MI",
    "EQ/Z", "C/ULT",
    "",      "GE",
    "GT",   "UGT",
    "PO",   "PL",
    "NE/NZ","NC/UGE"
};

char *R8_names[] = {
    "W","A","B","C","D","E","H","L",
};

char *R16_names[] = {
    "WA","BC","DE","HL","IX","IY","IZ","SP",
};

char *R32_names[] = {
    "XWA","XBC","XDE","XhL","XIX","XIY","XIZ","XSP",
};

char *addr_names[] = {
    "(XWA)","(XBC)","(XDE)","(XhL)","(XIX)","(XIY)","(XIZ)","(XSP)",
    "(XWA+","(XBC+","(XDE+","(XhL+","(XIX+","(XIY+","(XIZ+","(XSP+",
    "","","",
    "","","","","",""
};

char *r8_names[] = {
    "RA0","RW0","QA0","QW0",
    "RC0","RB0","QC0","QB0",
    "RE0","RD0","QE0","QD0",
    "RL0","RH0","QL0","QH0",
    "RA1","RW1","QA1","QW1",
    "RC1","RB1","QC1","QB1",
    "RE1","RD1","QE1","QD1",
    "RL1","RH1","QL1","QH1",
    "RA2","RW2","QA2","QW2",
    "RC2","RB2","QC2","QB2",
    "RE2","RD2","QE2","QD2",
    "RL2","RH2","QL2","QH2",
    "RA3","RW3","QA3","QW3",
    "RC3","RB3","QC3","QB3",
    "RE3","RD3","QE3","QD3",
    "RL3","RH3","QL3","QH3",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "A'","W'","QA'","QW'",
    "C'","B'","QC'","QB'",
    "E'","D'","QE'","QD'",
    "L'","H'","QL'","QH'",

    "A","W","QA","QW",
    "C","B","QC","QB",
    "E","D","QE","QD",
    "L","H","QL","QH",

    "IXL","IXh","QIXL","QIXh",
    "IYL","IYH","QIYL","QIYH",
    "IZL","IZH","QIZL","QIZH",
    "SPL","SPH","QSPL","QSPH",
};

char *r16_names[] = {
    "RWA0","?","QWA0","?",
    "RBC0","?","QBC0","?",
    "RDE0","?","QDE0","?",
    "RHL0","?","QHL0","?",
    "RWA1","?","QWA1","?",
    "RBC1","?","QBC1","?",
    "RDE1","?","QDE1","?",
    "RHL1","?","QHL1","?",
    "RWA2","?","QWA2","?",
    "RBC2","?","QBC2","?",
    "RDE2","?","QDE2","?",
    "RHL2","?","QHL2","?",
    "RWA3","?","QWA3","?",
    "RBC3","?","QBC3","?",
    "RDE3","?","QDE3","?",
    "RHL3","?","QHL3","?",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "WA'","?","QWA'","?",
    "BC'","?","QBC'","?",
    "DE'","?","QDE'","?",
    "HL'","?","QHL'","?",

    "WA","?","QWA","?",
    "BC","?","QBC","?",
    "DE","?","QDE","?",
    "HL","?","QHL","?",

    "IX","?","QIX","?",
    "IY","?","QIY","?",
    "IZ","?","QIZ","?",
    "SP","?","QSP","?",
};


char *r32_names[] = {
    "XWA0","?","?","?",
    "XBC0","?","?","?",
    "XDE0","?","?","?",
    "XhL0","?","?","?",

    "XWA1","?","?","?",
    "XBC1","?","?","?",
    "XDE1","?","?","?",
    "XhL1","?","?","?",

    "XWA2","?","?","?",
    "XBC2","?","?","?",
    "XDE2","?","?","?",
    "XhL2","?","?","?",

    "XWA3","?","?","?",
    "XBC3","?","?","?",
    "XDE3","?","?","?",
    "XhL3","?","?","?",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",
    "?","?","?","?", "?","?","?","?", "?","?","?","?", "?","?","?","?",

    "XWA'","?","?","?",
    "XBC'","?","?","?",
    "XDE'","?","?","?",
    "XhL'","?","?","?",

    "XWA","?","?","?",
    "XBC","?","?","?",
    "XDE","?","?","?",
    "XhL","?","?","?",

    "XIX","?","?","?",
    "XIY","?","?","?",
    "XIZ","?","?","?",
    "XSP","?","?","?",
};

unsigned char get8u( unsigned char *b ) { return *b; }
unsigned short get16u( unsigned char *b ) { return ((b[1] << 8) | *b); }
unsigned int get24u( unsigned char *b ) { return ((b[2] << 16) | (b[1] << 8) | *b); }
unsigned int get32u( unsigned char *b ) { return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | *b); }
char get8( unsigned char *b ) { return *b; }
short get16( unsigned char *b ) { return ((b[1] << 8) | *b); }
int get32( unsigned char *b ) { return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | *b); }
int getR( unsigned char *m ) { return *m & 0x07; }
int getr( unsigned char *m ) {
  if ((*m & 0x0f) == 0x07) {
		// opcode case xxxx0111 -> next byte tells the actual r
    return -1;
  } else {
    return *m & 0x07;
  }
}
int getzz( unsigned char *m ) { return (*m & 0x30) >> 4;}
int getzzz( unsigned char *m ) { return (*m & 0x70) >> 4; }
int getcc( unsigned char *m ) { return *m & 0x0f; }
int getmem( unsigned char *m ) {
	int mm = *m & 0x4f;
	return ((mm & 0x40) >> 2) | (mm & 0x0f);
}
int getz1( unsigned char *m ) { return *m & 0x02; }
int getz4( unsigned char *m ) {
	int z4 = *m & 0x30;
	return z4 >> 4;
}
int get3( unsigned char *m ) { return *m & 0x07; }
int get4( unsigned char *m ) { return *m & 0x0f; }

int retmem( unsigned char *b, char *s, int mem ) {
  int i;

  switch (mem) {
  case ARI_XWA: case ARI_XBC: case ARI_XDE: case ARI_XHL:
  case ARI_XIX: case ARI_XIY: case ARI_XIZ: case ARI_XSP:
    strcpy(s,addr_names[mem]);
    return 1;
  case ARID_XWA: case ARID_XBC: case ARID_XDE: case ARID_XHL:
  case ARID_XIX: case ARID_XIY: case ARID_XIZ: case ARID_XSP:
    sprintf(s,"%s%04Xh)",addr_names[mem],get8(b+1) );
    return 1+1;
  case ABS_B:
    sprintf(s,"(%02Xh)",b[1]);
    return 1+1;
  case ABS_W:
    sprintf(s,"(%04Xh)",get16u(b+1));
    return 1+2;
  case ABS_L:
    sprintf(s,"(%06Xh)",get24u(b+1));
    return 1+3;
	case ARI:
    switch (b[1] & 0x03) {
    case 0x00:  // (r32)
      sprintf(s,"(%s)",r32_names[b[1] & 0xfc]);
      return 1+1;
    case 0x01:  // (r32 + d16)
      sprintf(s,"(%s+%04Xh)",r32_names[b[1] & 0xfc ], get16(b+2) );
      return 1+3;
    case 0x03:
      if (b[1] & 0x4) { // (r32+r16)
        sprintf(s,"(%s+%s)",r32_names[b[2]],r16_names[b[3]]);
      } else {          // (r32+r8)
        sprintf(s,"(%s+%s)",r32_names[b[2]],r8_names[b[3]]);
      }
      return 1+3;
    }
  case ARI_PD:
    //i = b[1] & 0x03;
    //i = i ? i << 1 : 1;
    //sprintf(s,"(-%s:%d)",r32_names[b[1] & 0xfc],i);
    sprintf(s,"(-%s)",r32_names[b[1] & 0xfc]);
    return 1+1;
  case ARI_PI:
    //i = b[1] & 0x03;
    //i = i ? i << 1 : 1;
    //sprintf(s,"(%s+:%d)",r32_names[b[1] & 0xfc],i);
    sprintf(s,"(%s+)",r32_names[b[1] & 0xfc]);
    return 1+1;
  default:
    strcpy(s,"(\?\?\?)");  // Invalid..
    break;
  }
  return 0;
}

char **getregs( int r, int zz ) {
  if (r < 0) {
    switch (zz) {
    case 0x00:
      return r8_names;
    case 0x01:
      return r16_names;
    case 0x02:
      return r32_names;
    default:
      break;
    }
  } else {
    switch (zz) {
    case 0x00:
      return R8_names;
    case 0x01:
      return R16_names;
    case 0x02:
      return R32_names;
    default:
      break;
    }
  }
  return NULL;
}

// Constructor
tlcs900hdebugger::tlcs900hdebugger(void)
{
	m_debugState = DEBUGGER_IDLE;
	for ( int i = 0; i < 100; i++)
	{
		m_breakpointList[i].active = false; // set all breakpoints to nothing
	}

	// cr1eate a page of character memory (speed hack)
	bufPage = new char[0x200000 * MAX_INSTR_LEN];
	for ( int i = 0; i < 0x200000; i++)
	{
		bufPage[i*MAX_INSTR_LEN] = 0;
	}
}

// Destructor
tlcs900hdebugger::~tlcs900hdebugger(void)
{
	// Delete our page
	delete bufPage;
}

int tlcs900hdebugger::getInc(unsigned int addr)
{
	return bufInc[(addr-0x200000)];
}

// Get the string pointer for the specific address
char * tlcs900hdebugger::getBufString(unsigned int addr)
{
	return &bufPage[(addr-0x200000)*MAX_INSTR_LEN];;
}

// Get the memory to the breakpoint list
tlcs900hBreakpoint * tlcs900hdebugger::getBreakpointList()
{
	return &(m_breakpointList[0]);
}

// Set our breakpoint
int tlcs900hdebugger::setBreakpoint(unsigned int address, unsigned char type)
{
	int i;
	// Check for a previously created breakpoint with this address first
	for ( i = 0; i < 100; i++ )
	{
		if ( m_breakpointList[i].address == address && m_breakpointList[i].type == type)
		{
			return -1;
		}
	}
	for ( i = 0; i < 100; i++ )
	{
		if ( m_breakpointList[i].active == false )
		{
			m_breakpointList[i].address = address;
			m_breakpointList[i].type = type;
			m_breakpointList[i].active = true;
			return i; // return the breakpoint we set
		}
	}
	return -1; // nothing found
}

// Remove a breakpoint
void tlcs900hdebugger::removeBreakpoint(int index)
{
	if ( index >= 0 && index < 100 )
	{
		m_breakpointList[index].active = false;
	}
}

void tlcs900hdebugger::clearBreakpoints()
{
	for(int i = 0; i < 100; i++)
	{
		m_breakpointList[i].active = false;
	}
}

// break is different, because the debugger needs to update
void tlcs900hdebugger::breakp()
{
	m_debugState = DEBUGGER_BREAK;
}

// pause the running tlcs900h emulation
void tlcs900hdebugger::pause()
{
	m_debugState = DEBUGGER_PAUSE;
}

// step to the next opcode
void tlcs900hdebugger::stepin()
{
	m_debugState = DEBUGGER_STEPIN;
}

// step-over to the next opcode
void tlcs900hdebugger::stepover()
{
	m_debugState = DEBUGGER_STEPOVER;
}

// continue where we left off
void tlcs900hdebugger::resume()
{
	// We don't need to step ahead to resume state, go directly to idle
	m_debugState = DEBUGGER_IDLE;
}

// clear the debugger flags
void tlcs900hdebugger::clear()
{
	m_debugState = DEBUGGER_IDLE;
}

// what state is our debugger in?
unsigned char tlcs900hdebugger::state()
{
	return m_debugState;
}

// get debugger address
unsigned char * tlcs900hdebugger::getCodePtr(unsigned long addr)
{
	addr &= 0x00FFFFFF;
	if (addr<0x00200000)
	{
		if (addr<0x000008a0) // why is this 0x8A0 and not 0xFF???
		{
			return &memCPURAM[addr];//cpuram[addr];
		}
		if (addr>0x00003fff && addr<0x00018000)
        {
            return &memRAM[addr-0x00004000];
        }
	}
	else
	{
		if (addr<0x00400000)
		{
            return &memROM[(addr-0x00200000)];
		}
        if(addr<0x00800000) //Flavor added
		{
            return 0;
		}
		if (addr<0x00A00000)
		{
            return &mem32ROM[(addr-0x00800000)];
		}
		if(addr<0x00FF0000) //Flavor added
		{
            return 0;
		}
        return &memBios[addr-0x00ff0000];
	}
	return 0;
}


// Decode the ROM at given address
unsigned int tlcs900hdebugger::decodeTlcs900h(const unsigned int addr)	// decode the current PC
{	
	//unsigned char * b = dd->buffer + dd->pos;

	// Get our instruction buffer
	char * instrBuf = &bufPage[(addr-0x200000)*MAX_INSTR_LEN];

	char empty[] = "            ";
	unsigned char * b = getCodePtr(addr);
	unsigned char mem;
	unsigned char zz; // ??
	int n;
	int len, s, j;

	struct tlcs900d * dd = &m_dd;
	dd->addr = addr;
		
	len = decodeFixed(dd);
	if ( len )
		goto print_op;
	
	mem = getmem(b);
	zz = getzz(b);

	if ( *b >= 0x80 )
	{
		// 0x03 is our specialty case
		if ( zz == 0x03 )
		{
			len = decode_B0_mem(dd);
		}
		else
		{
			//
            // These are opcodes like: 
            //   E8+r
            //   D8+r
            //   C8+zz+r
            //   85+zz
            //   83+zz
            //   80+zz+mem
            //   80+zz+R
            //

            if (mem >= 0x17 && mem <= 0x1f) {
                len = decode_zz_r(dd);
            } else if (mem <= 0x07 && b[1] >= 0x10 && b[1] <= 0x17) {
                len = decode_zz_R(dd);
            } else if (mem <= 0x15) {
                len = decode_zz_mem(dd);
            }
		}
	}
	else
	{
        len = decode_xx(dd);
    }

	print_op:

	if (len == 0) {
		sprintf(instrBuf,"%02X                ?????\n",*b);
		return 1;
	} else {
		switch (dd->opt) {
			case OPT_1_0_0:
				sprintf(instrBuf,"%02X                ",*b);
			break;
			case OPT_1_1_0:
				sprintf(instrBuf,"%02X %02X             ",*b, *(b+1));
			break;
			case OPT_1_1_1:
				sprintf(instrBuf,"%02X %02X %02X          ",*b,*(b+1),*(b+2));
			break;
			case OPT_1_1_2:
				sprintf(instrBuf,"%02X %02X %02X%02X        ",*b,*(b+1),*(b+2),*(b+3));
			break;
			case OPT_1_2_0:
				sprintf(instrBuf,"%02X %02X%02X           ",*b,*(b+1),*(b+2));
			break;
			case OPT_1_3_0:
				sprintf(instrBuf,"%02X %02X%02X%02X         ",*b,*(b+1),*(b+2),*(b+3));
			break;
			case OPT_2_1_2:
				sprintf(instrBuf,"%02X%02X %02X %02X%02X      ",*b,*(b+1),*(b+2),*(b+3),*(b+4));
			break;
			case OPT_1_4_0:
				sprintf(instrBuf,"%02X %02X%02X%02X%02X       ",*b,*(b+1),*(b+2),*(b+3),*(b+4));
			break;
			case OPT_1_1_1_1_1_1:
				sprintf(instrBuf,"%02X %02X %02X %02X %02X %02X ",*b,*(b+1),*(b+2),
					*(b+3),*(b+4),*(b+5));
			break;
			case OPT_1_n_1:
				sprintf(instrBuf,"%02X ",*b++);
				for (s = 0, j = 2; j < len; j++, s += 2) { sprintf(instrBuf,"%s%02X",instrBuf,*b++); }
				if (s) { sprintf(instrBuf,"%s ",instrBuf); s++; }
				s = 18 - 5 - s;
				sprintf(instrBuf,"%s%02X%*.*s",instrBuf,*b++,s,s,empty);
			break;
			case OPT_1_n_1_1:
				sprintf(instrBuf,"%02X ",*b++);
				for (s = 0, j = 3; j < len; j++, s += 2) { sprintf(instrBuf,"%s%02X",instrBuf,*b++); }
				if (s) { printf(" "); s++; }
				s = 18 - 8 - s;
				sprintf(instrBuf,"%s%02X %02X%*.*s",instrBuf,*(b),*(b+1),s,s,empty);
			break;
			case OPT_1_n_1_2:
				sprintf(instrBuf,"%02X ",*b++);
				for (s = 0, j = 4; j < len; j++, s += 2) { sprintf(instrBuf,"%s%02X",instrBuf,*b++); }
				if (s) { sprintf(instrBuf,"%s ",instrBuf); s++; }
				s = 18 - 10 - s;
				sprintf(instrBuf,"%s%02X %02X%02X%*.*s",instrBuf,*(b),*(b+1),*(b+2),s,s,empty);
			break;
			case OPT_1_n_2:
				sprintf(instrBuf,"%02X ",*b++);
				for (s = 0, j = 3; j < len; j++, s += 2) { sprintf(instrBuf,"%s%02X",instrBuf,*b++); }
				if (s) { sprintf(instrBuf,"%s ",instrBuf); s++; }
				s = 18 - 7 - s;
				sprintf(instrBuf,"%s%02X%02X%*.*s",instrBuf,*(b),*(b+1),s,s,empty);
			break;
			case OPT_1_n_1_4:
				sprintf(instrBuf,"%02X ",*b++);
				for (s = 0, j = 6; j < len; j++, s += 2) { sprintf(instrBuf,"%s%02X",instrBuf,*b++); }
				if (s) { printf(" "); b++; }
				s = 18 - 14 - s;
				sprintf(instrBuf,"%s%02X %02X%02X%02X%02X%*.*s",
					instrBuf,*(b),*(b+1),*(b+2),*(b+3),*(b+4),s,s,empty);
			break;
			default:
				//assert(0);
			break;
		}

		sprintf(instrBuf,"%s%-5s %s\n",instrBuf,dd->opf,dd->ops);
		//dd->pos += len;
	}

	//return bytesRead;
	bufInc[(addr-0x200000)] = len;

	return len;
}

// Decode any of the possible fixed opcodes
int tlcs900hdebugger::decodeFixed(struct tlcs900d *dd)
{
	unsigned char * b = getCodePtr(dd->addr);//dd->buffer + dd->pos;
	int len = 1;
	enum opcodes op;
	int d;

	*dd->ops = '\0';
	dd->opt = OPT_1_0_0;
	op = INVALID;

	switch (*b) {
		case 0x08: // LD (#8),#8
			op = LD;
			sprintf(dd->ops,"(%02Xh),%03Xh",get8u(b+1),get8u(b+2) );
			len = 1+2;
			dd->opt = OPT_1_1_1;
		break;
		case 0x0a: // LDW (#8),#16
			op = LDW;
			sprintf(dd->ops,"(%02Xh),%05Xh",get8u(b+1),get16u(b+2) );
			len = 1+3;
			dd->opt = OPT_1_1_2;
		break;
		case 0x09: // PUSH #8
			op = PUSH;
			sprintf(dd->ops,"%03Xh",get8u(b+1) );
			len = 1+1;
			dd->opt = OPT_1_1_0;
		break;
		case 0x0b: // PUSHW #16
			op = PUSHW;
			sprintf(dd->ops,"%05Xh",get16u(b+1) );
			len = 1+2;
			dd->opt = OPT_1_2_0;
		break;
		case 0x18: // PUSH F
			op = PUSH;
			sprintf(dd->ops,"F");
		break;
		case 0x14: // PUSH A
			op = PUSH;
			sprintf(dd->ops,"A");
		break;
		case 0x19: // POP F
			op = POP;
			sprintf(dd->ops,"F");
		break;
		case 0x15: // POP A
			op = POP;
			sprintf(dd->ops,"A");
		break;
		case 0x16: // EX F,F'
			op = EX;
			sprintf(dd->ops,"F,F'");
		break;
		case 0x10: // RCF
			op = RCF;
		break;
		case 0x11: // SCF
			op = SCF;
		break;
		case 0x12: // CCF
			op = CCF;
		break;
		case 0x13: // ZCF
			op = ZCF;
		break;
		case 0x00: // NOP
			op = NOP;
		break;
		case 0x06: // EI [#3] / DI
			if (b[1] == 0x07) {
				op = DI;
			} else {
				op = EI;
				sprintf(dd->ops,"%d",b[1] & 0x07);
			}
			len = 1+1;
			dd->opt = OPT_1_1_0;
		break;
		case 0x02: // PUSH SR
			op = PUSH;
			sprintf(dd->ops,"SR");
		break;
		case 0x03: // POP SR
			op = POP;
			sprintf(dd->ops,"SR");
		break;
		case 0x05: // HALT
			op = HALT;
		break;
		case 0x0c: // INCF
			op = INCF;
		break;
		case 0x0d: // DECF
			op = DECF;
		break;
		case 0x1a: // JP #16
			op = JP;
			sprintf(dd->ops,"%05Xh",get16u(b+1));
			len = 1+2;
			dd->opt = OPT_1_2_0;
		break;
		case 0x1b: // JP #24
			op = JP;
			sprintf(dd->ops,"%07Xh",get24u(b+1));
			len = 1+3;
			dd->opt = OPT_1_3_0;
		break;
		case 0x1c: // CALL #16
			op = CALL;
			sprintf(dd->ops,"%05Xh",get16u(b+1));
			len = 1+2;
			dd->opt = OPT_1_2_0;
		break;
		case 0x1d: // CALL #24
			op = CALL;
			sprintf(dd->ops,"%07Xh",get24u(b+1));
			len = 1+3;
			dd->opt = OPT_1_3_0;
		break;
		case 0x1e: // CALR d16 !!!!
			op = CALR;
			d = dd->addr + 3 + get16(b+1); //dd->base + dd->pos + 3 + get16(b+1);
			sprintf(dd->ops,"%07Xh",d);   // relative to the address..
			len = 1+2;
			dd->opt = OPT_1_2_0;
		break;
		case 0x0e: // RET
			op = RET;
		break;
		case 0x0f: // RETD d16
			op = RETD;
			sprintf(dd->ops,"%05Xh",get16(b+1));
			len = 1+2;
			dd->opt = OPT_1_2_0;
		break;
		case 0x07: // RETI
			op = RETI;
		break;
		case 0xf7: // LDX (#8),#8
			op = LDX;
			sprintf(dd->ops,"(%02Xh),%03Xh",get8u(b+2),get8u(b+4));
			len = 1+5;
			dd->opt = OPT_1_1_1_1_1_1;
		break;
		case 0xf8: // SWI [#3];
		case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			op = SWI;
			sprintf(dd->ops,"%d",*b & 0x07);
		break;
		case 0x17: // LDF #3
			op = LDF;
			sprintf(dd->ops,"%d",get8u(b+1) & 0x07);
			len = 1+1;
			dd->opt = OPT_1_1_0;
		break;
		default:
			len = 0;
		break;
	}

	dd->opf = opcode_names[op];

	return len;
}

// Decode with the B0 mem
int tlcs900hdebugger::decode_B0_mem( struct tlcs900d * dd ) {
	char m[OPS_LEN];
	enum opcodes op = INVALID;
	unsigned char *b = getCodePtr(dd->addr);//dd->buffer + dd->pos;
	int len = 1;
	int n;
	int zz;
	int R;

	*dd->ops = '\0';
	dd->opt = OPT_1_n_1;

	//
	// Following opcodes:
	//   LD (mem),R         B0+mem:40+zz+R  +
	//   LD<W> (mem),#      B0+mem:00+z:#   +
	//   LD<W> (mem),(#16)  B0+mem:14+z:#16 +
	//   POP<W> (mem)       B0+mem:04+z     +
	//   LDA R,mem          B0+mem:20+s+R   +
	//   LDAR R,$+4+d16     F3:13:d16:20+s+R    +
	//   LDCF #3,(mem)      B0+mem:98+#3    +
	//   LDCF A,(mem)       B0+mem:2B       +
	//   STCF #3,(mem)      B0+mem:A0+#3    +
	//   STCF A,(mem)       B0+mem:2C       +
	//   ANDCF #3,(mem)     B0+mem:80+#3    +
	//   ANDCF A,(mem)      B0+mem:28       +
	//   ORCF #3,(mem)      B0+mem:88+#3    +
	//   ORCF A,(mem)       B0+mem:29       +
	//   XORCF #3,(mem)     B0+mem:90+#3    +
	//   XORCF A,(mem)      B0+mem:2A       +
	//   BIT #3,(mem)       B0+mem:C8+#3    +
	//   RES #3,(mem)       B0+mem:B0+#3    +
	//   SET #3,(mem)       B0+mem:B8+#3    +
	//   CHG #3,(mem)       B0+mem:C0+#3    +
	//   TSET #3,(mem)      B0+mem:A8+#3    +
	//   JP [cc,]mem        B0+mem:D0+cc    +
	//   CALL [cc,]mem      B0+mem:E0+cc    +
	//   RET cc             B0:F0+cc        +
	//

	if (*b == 0xf3 && b[1] == 0x13) {  // LDAR..
		int d16 = get16(b+2) + 4 + dd->addr; //dd->base + dd->pos;

		dd->opf = opcode_names[LDAR];

		if (b[4] & 0x20) {
			sprintf(dd->ops,"%s,%09Xh",R32_names[ getR(b+4) ],d16);
		} else {
			sprintf(dd->ops,"%s,%09Xh",R16_names[ getR(b+4) ],d16);
		}
		dd->opt = OPT_2_1_2;
		return 5;
	}
	if (*b == 0xb0 && b[1] >= 0xf0) {  // RET cc..
		dd->opf = opcode_names[RET];
		strcpy(dd->ops,cc_names[getcc(b+1)]);
		dd->opt = OPT_1_1_0;
		return 2;
	}

	// Another huge switch based on the nth byte..

	n = retmem(b,m,getmem(b));
	if (n == 0) { return 0; }

	switch (b[n]) {
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
		case 0xec: case 0xed: case 0xee: case 0xef:
			dd->opf = opcode_names[CALL];
			if (getcc(b+n) != 8) {
				sprintf(dd->ops,"%s,%s",cc_names[getcc(b+n)],m);
			} else {
				strcpy(dd->ops,m);
			}
			dd->opt = OPT_1_n_1;
			return n+1;
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf:
			dd->opf = opcode_names[JP];
			if (getcc(b+n) != 8) {
				sprintf(dd->ops,"%s,%s",cc_names[getcc(b+n)],m);
			} else {
				strcpy(dd->ops,m);
			}
			dd->opt = OPT_1_n_1;
			return n+1;
		case 0x28:  // ANDCF A,(mem)
			dd->opf = opcode_names[ANDCF];
			sprintf(dd->ops,"A,%s",m);      
			return n+1;
		case 0x29:  // ORCF A,(mem)
			dd->opf = opcode_names[ORCF];
			sprintf(dd->ops,"A,%s",m);      
			return n+1;
		case 0x2a:  // XORCF A,(mem)
			dd->opf = opcode_names[XORCF];
			sprintf(dd->ops,"A,%s",m);      
			return n+1;
		case 0x2b:  // LDCF A,(mem)
			dd->opf = opcode_names[LDCF];
			sprintf(dd->ops,"A,%s",m);      
			return n+1;
		case 0x2c:  // STCF A,(mem)
			dd->opf = opcode_names[STCF];
			sprintf(dd->ops,"A,%s",m);      
			return n+1;
		case 0x00:  // LD (mem),#8
			dd->opf = opcode_names[LD];
			sprintf(dd->ops,"%s,%03Xh",m,get8u(b+n+1));      
			dd->opt = OPT_1_n_1_1;
			return n+2;
		case 0x02:  // LDW (mem),#16
			dd->opf = opcode_names[LDW];
			sprintf(dd->ops,"%s,%05Xh",m,get16u(b+n+1));      
			dd->opt = OPT_1_n_1_2;
			return n+3;
		case 0x04:  // POP (mem)
			dd->opf = opcode_names[POP];
			strcpy(dd->ops,m);
			return n+1;
		case 0x06:  // POPW (mem)
			dd->opf = opcode_names[POPW];
			strcpy(dd->ops,m);
			return n+1;
		case 0x14:  // LD (mem),(#16)
			dd->opf = opcode_names[LD];
			sprintf(dd->ops,"%s,(%04Xh)",m,get16u(b+n+1) );      
			return n+3;
		case 0x16:  // LDW (mem),(#16)
			dd->opf = opcode_names[LDW];
			sprintf(dd->ops,"%s,(%04Xh)",m,get16u(b+n+1) );      
			dd->opt = OPT_1_n_2;
			return n+3;
		default:
		break;
	}

	zz = getzz(b+n);
	R = getR(b+n);

	switch (b[n] & 0xf8) {
		case 0x40:  // LD (mem),R
			op = LD;
			sprintf(dd->ops,"%s,%s",m,R8_names[R]);
			len = n+1;
		break;
		case 0x50:  // LD (mem),R
			op = LD;
			sprintf(dd->ops,"%s,%s",m,R16_names[R]);
			len = n+1;
		break;
			case 0x60:  // LD (mem),R
			op = LD;
			sprintf(dd->ops,"%s,%s",m,R32_names[R]);
			len = n+1;
		break;
		case 0x20:
			op = LDA; // LDA R,(mem)
			sprintf(dd->ops,"%s,%s",R16_names[R],m);
			len = n+1;
		break;
		case 0x30:
			op = LDA; // LDA R,(mem)
			sprintf(dd->ops,"%s,%s",R32_names[R],m);
			len = n+1;
		break;
		case 0x98:  // LDCF #3,(mem)
			op = LDCF;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xa0:  // STCF #3,(mem)
			op = STCF;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0x80:  // ANDCF #3,(mem)
			op = ANDCF;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0x88:
			op = ORCF;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0x90:
			op = XORCF;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xc8:
			op = BIT;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xb0:
			op = RES;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xb8:
			op = SET;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xc0:
			op = CHG;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		case 0xa8:
			op = TSET;
			sprintf(dd->ops,"%d,%s",R,m);  // R is now #3
			len = n+1;
		break;
		default:  // INVALID
			len = 0;
			op = INVALID;
		break;
	}

	//
	dd->opf = opcode_names[op];
	return len;
}


int tlcs900hdebugger::decode_xx( struct tlcs900d *dd ) {
	unsigned char *b = getCodePtr(dd->addr);//dd->buffer + dd->pos;
	int len = 0;
	int cc = getcc(b);
	int R = getR(b);
	enum opcodes op = INVALID;
	int d = dd->addr; //dd->base + dd->pos;

	*dd->ops = '\0';
	dd->opt = OPT_1_0_0;

	//
	// Only few opcodes left..
	//   LD R,#             0+zzz+R:#   %0zzz0RRR:#
	//                                  %00100RRR
	//                                  %00110RRR
	//                                  %01000RRR
	//   PUSH R             28+s+R      %001s1RRR
	//   POP R              48+s+R      %010s1RRR
	//   JR [cc,]$+2+d8     60+cc:d8    %0110cccc:d8
	//   JRL [cc,]$+3+d16   70+cc:d16   %0111cccc:d16
	//

	switch (*b & 0xf8) {
		case 0x20:  // LD R,#8
			op = LD;
			dd->opt = OPT_1_1_0;
			sprintf(dd->ops,"%s,%03Xh",R8_names[R],get8u(b+1) );
			len = 1+1;
		break;
		case 0x30:  // LD R,#16
			op = LD;
			dd->opt = OPT_1_2_0;
			sprintf(dd->ops,"%s,%05Xh",R16_names[R],get16u(b+1) );
			len = 1+2;
		break;
		case 0x40:  // LD R,#32
			op = LD;
			dd->opt = OPT_1_4_0;
			sprintf(dd->ops,"%s,%09Xh",R32_names[R],get32u(b+1) );
			len = 1+4;
		break;
		case 0x28:  // PUSH R (word)
			op = PUSH;
			sprintf(dd->ops,"%s",R16_names[R] );
			len = 1;
		break;
		case 0x38:  // PUSH R (long)
			op = PUSH;
			sprintf(dd->ops,"%s",R32_names[R] );
			len = 1;
		break;
		case 0x48:  // POP R (word)
			op = POP;
			sprintf(dd->ops,"%s",R16_names[R] );
			len = 1;
		break;
		case 0x58:  // POP R (long)
			op = POP;
			sprintf(dd->ops,"%s",R32_names[R] );
			len = 1;
		break;
		default:
			if ((*b & 0x70) == 0x70) {
				//   JRL [cc,]$+3+d16   70+cc:d16   %0111cccc:d16
				d = d + get16(b+1) + 3;
				op = JRL; 
				len = 1+2;
				dd->opt = OPT_1_2_0;
			} else if ((*b & 0x60) == 0x60) {
				//   JR [cc,]$+2+d8     60+cc:d8    %0110cccc:d8
				d = d + get8(b+1) + 2;
				op = JR;
				len = 1+1;
				dd->opt = OPT_1_1_0;
			} else {
				// unknown instruction..
				op = INVALID;
				break;
			}

			// do JR or JRL..
			if (cc != 0x08) {
				sprintf(dd->ops,"%s,%07Xh",cc_names[cc],d);
			} else {
				sprintf(dd->ops,"%07Xh",d);
			}
		break;
	}

	dd->opf = opcode_names[op];
	return len;
}

int tlcs900hdebugger::decode_zz_r( struct tlcs900d *dd ) {
	unsigned char *b = getCodePtr(dd->addr); //dd->buffer + dd->pos;
	unsigned char c;
	char **regs;
	char **Regs;
	int zz = getzz(b);
	int r = getr(b);
	int len = 1;
	enum opcodes op = INVALID;
	int base;

	switch (zz) {
		case 0x00:
			Regs = R8_names;
		break;
		case 0x01:
			Regs = R16_names;
		break;
		case 0x02:
			Regs = R32_names;
		break;
		case 0x03:
		default:
		return 0;
	}

	//
	regs = getregs( r, zz );

	if (r < 0) {
		r = b[len++];
		dd->opt = OPT_1_1_1;
	}

	//
	// Following many opcodes:
	//   LD r,#     C8+zz+r:03:#    * 0x03-0x10
	//   PUSH r     C8+zz+r:04      *
	//   POP r      C8+zz+r:05      *
	//   CPL r      C8+zz+r:06      *
	//   NEG r      C8+0z+r:07      *
	//   MUL rr,#   C8+zz+r:08:#    *
	//   MULS rr,#  C8+zz+r:09:#    *
	//   DIV rr,#   C8+zz+r:0A:#    *
	//   DIVS rr,#  C8+zz+r:0B:#    *
	//   LINK r,d16 C8+10+r:0C:d16  *
	//   UNLK r     C8+10+r:0D      *
	//   BS1F A,r   C8+01+r:0E      *
	//   BS1B A,r   C8+01+r:0F      *
	//   DAA r      C8+00+r:10      *
	//   EXTZ r     C8+zz+r:12      * 0x12-0x14
	//   EXTS r     C8+zz+r:13      *
	//   PAA r      C8+zz+r:14      *
	//   MIRR r     C8+01+r:16      * 0x16
	//   MULA rr    C8+01+r:19      * 0x19
	//   DJNZ [r,]$+3/4+d8  C8+zz+r:1C:d8 * 0x1c
	//   ANDCF #4,r C8+zz+r:20:#4   * 0x20-0x24
	//   ORCF #4,r  C8+zz+r:21:#4   *
	//   XORCF #4,r C8+zz+r:22:#4   *
	//   LDCF #4,r  C8+zz+r:23:#4   *
	//   STCF #4,r  C8+zz+r:24:#4   *
	//   ANDCF A,r  C8+zz+r:28      * 0x28-0x2c
	//   ORCF A,r   C8+zz+r:29      *
	//   XORCF A,r  C8+zz+r:2A      *
	//   LDCF A,r   C8+zz+r:2B      *
	//   STCF A,r   C8+zz+r:2C      *
	//   LDC cr,r   C8+zz+r:2E      *
	//   LDC r,cr   C8+zz+r:2F      *
	//   RES #4,r   C8+zz+r:30:#4   * 0x30-0x34
	//   SET #4,r   C8+zz+r:31:#4   *
	//   CHG #4,r   C8+zz+r:32:#4   *
	//   BIT #4,r   C8+zz+r:33:#4   *
	//   TSET #4,r  C8+zz+r:34:#4   *
	//   MINC1 #,r  C8+01+r:38:#-1  * 0x38-0x3a
	//   MINC2 #,r  C8+01+r:39:#-2  *
	//   MINC4 #,r  C8+01+r:3A:#-4  *
	//   MDEC1 #,r  C8+01+r:3C:#-1  * 0x3c-0x3e
	//   MDEC2 #,r  C8+01+r:3D:#-2  *
	//   MDEC4 #,r  C8+01+r:3E:#-4  *
	//   ADD r,#    C8+zz+r:C8:#    * 0xc8-0xcf
	//   ADC r,#    C8+zz+r:C9:#    *
	//   SUB r,#    C8+zz+r:CA:#    *
	//   SBC r,#    C8+zz+r:CB:#    *
	//   AND r,#    C8+zz+r:CC:#    *
	//   XOR r,#    C8+zz+r:CD:#    *
	//   OR r,#     C8+zz+r:CE:#    *
	//   CP r,#     C8+zz+r:CF:#    *
	//   RLC #4,r   C8+zz+r:E8:#4   * 0xe8-0xef
	//   RRC #4,r   C8+zz+r:E9:#4   *
	//   RL #4,r    C8+zz+r:EA:#4   *
	//   RR #4,r    C8+zz+r:EB:#4   *
	//   SLA #4,r   C8+zz+r:EC:#4   *
	//   SRA #4,r   C8+zz+r:ED:#4   *
	//   SLL #4,r   C8+zz+r:EE:#4   *
	//   SRL #4,r   C8+zz+r:EF:#4   *
	//   RLC A,r    C8+zz+r:F8      * 0xf8-0xff
	//   RRC A,r    C8+zz+r:F9      *
	//   RL A,r     C8+zz+r:FA      *
	//   RR A,r     C8+zz+r:FB      *
	//   SLA A,r    C8+zz+r:FC      *
	//   SRA A,r    C8+zz+r:FD      *
	//   SLL A,r    C8+zz+r:FE      *
	//   SRL A,r    C8+zz+r:FF      *
	// groups or 8s or 16s
	//   MUL RR,r   C8+zz+r:40+R    * 0x40-0xc7
	//   MULS RR,r  C8+zz+r:48+R    *
	//   DIV RR,r   C8+zz+r:50+R    *
	//   DIVS RR,r  C8+zz+r:58+R    *
	//   INC #3,r   C8+zz+r:60+#3   *
	//   DEC #3,r   C8+zz+r:68+#3   *
	//   SCC cc,r   C8+zz+r:70+cc   *
	//   ADD R,r    C8+zz+r:80+R    *
	//   LD R,r     C8+zz+r:88+R    *
	//   ADC R,r    C8+zz+r:90+R    *
	//   LD r,R     C8+zz+r:98+R    *
	//   SUB R,r    C8+zz+r:A0+R    *
	//   LD r,#3    C8+zz+r:A8+#3   *
	//   SBC R,r    C8+zz+r:B0+R    *
	//   EX R,r     C8+zz+r:B8+R    *
	//   AND R,r    C8+zz+r:C0+R    *
	//   XOR R,r    C8+zz+r:D0+R    * 0xd0-0xe7
	//   CP r,#3    C8+zz+r:D8+#3   *
	//   OR R,r     C8+zz+r:E0+R    *
	//   CP R,r     C8+zz+r:F0+R    * 0xf0-0xf7
	//   

	c = b[len++];

	if (c >= 0x40 && c < 0xc8 || c >= 0xd0 && c < 0xe8 || c >= 0xf0 && c < 0xf8) {
		// xx+R , xx+#3 , 70+cc 
		switch (c & 0xf8) {
			case 0x40:  // MUL RR,r
				op = MUL;
				goto zz_R;
			case 0x48:  // MULS RR,r
				op = MULS;
				goto zz_R;
			case 0x50:  // DIV RR,r
				op = DIV;
				goto zz_R;
			case 0x58:  // DIVS RR,r
				op = DIVS;
				goto zz_R;
			case 0x80:  // ADD R,r
				op = ADD;
				goto zz_R;
			case 0x88:  // LD R,r
				op = LD;
				goto zz_R;
			case 0x90:  // ADC R,r
				op = ADC;
				goto zz_R;
			case 0xa0:  // SUB R,r
				op = SUB;
				goto zz_R;
			case 0xb0:  // SBC R,r
				op = SBC;
				goto zz_R;
			case 0xb8:  // EX R,r
				op = EX;
				goto zz_R;
			case 0xc0:  // AND R,r
				op = AND;
				goto zz_R;
			case 0xd0:  // XOR R,r
				op = XOR;
				goto zz_R;
			case 0xe0:  // OR R,r
				op = OR;
				goto zz_R;
			case 0xf0:  // CP R,r
				op = CP;
			zz_R:
				sprintf(dd->ops,"%s,%s",Regs[c & 0x07],regs[r]);
				dd->opt = OPT_1_n_1;
			break;
			case 0x98:  // LD r,R
				op = LD;
				sprintf(dd->ops,"%s,%s",regs[r],Regs[c & 0x07]);
				dd->opt = OPT_1_n_1;
			break;
			case 0x60:  // INC #3,r
				op = INC;
			case 0x68:  // DEC #3,r
				if (op == INVALID) { op = DEC; }
				if ((c &= 0x07) == 0) { c = 8; }
				sprintf(dd->ops,"%d,%s",c,regs[r]);
				dd->opt = OPT_1_1_0;
			break;
			case 0xa8:  // LD r,#3
				op = LD;
			case 0xd8:  // CP r,#3
				if (op == INVALID) { op = CP; }
				sprintf(dd->ops,"%s,%03Xh",regs[r],c & 0x07);
				dd->opt = OPT_1_1_0;
			break;
			case 0x70:  // SCC cc,r
			case 0x78: 
				op = SCC;
				sprintf(dd->ops,"%s,%s",cc_names[c & 0x0f],regs[r]);
				dd->opt = OPT_1_n_1;
			break;
			default:
				// unknown opcode..
				return 0;
		}
	} else {
		switch (c) {
			case 0x03:  // LD r,#
				op = LD;
				goto r_num;
			case 0x08:  // MUL rr,#
				op = MUL;
				goto r_num;
			case 0x09:  // MULS rr,#
				op = MULS;
				goto r_num;
			case 0x0a:  // DIV rr,#
				op = DIV;
				goto r_num;
			case 0x0b:  // DIVS rr,#
				op = DIVS;
				goto r_num;
			case 0xc8:  // ADD r,#
				op = ADD;
				goto r_num;
			case 0xc9:  // ADC r,#
				op = ADC;
				goto r_num;
			case 0xca:  // SUB r,#
				op = SUB;
				goto r_num;
			case 0xcb:  // SBC r,#
				op = SBC;
				goto r_num;
			case 0xcc:  // AND r,#
				op = AND;
				goto r_num;
			case 0xcd:  // XOR r,#
				op = XOR;
				goto r_num;
			case 0xce:  // OR r,#
				op = OR;
				goto r_num;
			case 0xcf:  // CP r,#
				op = CP;
			r_num:
				if (zz == 0) {
					sprintf(dd->ops,"%s,%03Xh",regs[r],get8u(b+len));
					len++;
					dd->opt = OPT_1_n_1_1;
				} else if (zz == 1) {
					sprintf(dd->ops,"%s,%05Xh",regs[r],get16u(b+len));
					len += 2;
					dd->opt = OPT_1_n_1_2;
				} else if (zz == 2) {
					sprintf(dd->ops,"%s,%09Xh",regs[r],get32u(b+len));
					len += 4;
					dd->opt = OPT_1_n_1_4;
				} else {
					// Hmmm not a valid instruction..
					return 0;
					//assert(zz < 3);
				}
			break;

			case 0x20:  // ANDCF #4,r
				op = ANDCF;
				goto num_r;
			case 0x21:  // ORCF #4,r
				op = ORCF;
				goto num_r;
			case 0x22:  // XORCF #4,r
				op = XORCF;
				goto num_r;
			case 0x23:  // LDCF #4,r
				op = LDCF;
				goto num_r;
			case 0x24:  // STCF #4,r
				op = STCF;
				goto num_r;
			case 0x30:  // RES #4,r
				op = RES;
				goto num_r;
			case 0x31:  // SET #4,r
				op = SET;
				goto num_r;
			case 0x32:  // CHG #4,r
				op = CHG;
				goto num_r;
			case 0x33:  // BIT #4,r
				op = BIT;
				goto num_r;
			case 0x34:  // TSET #4,r
				op = TSET;
				goto num_r;
			case 0xe8:  // RLC #4,r
				op = RLC;
				goto num_r;
			case 0xe9:  // RRC #4,r
				op = RRC;
				goto num_r;
			case 0xea:  // RL #4,r
				op = RL;
				goto num_r;
			case 0xeb:  // RR #4,r
				op = RR;
				goto num_r;
			case 0xec:  // SLA #4,r
				op = SLA;
				goto num_r;
			case 0xed:  // SRA #4,r
				op = SRA;
				goto num_r;
			case 0xee:  // SLL #4,r
				op = SLL;
				goto num_r;
			case 0xef:  // SRL #4,r
				op = SRL;
			num_r:
				sprintf(dd->ops,"%03Xh,%s", b[len++] & 0x0f, regs[r]);
				dd->opt = OPT_1_n_1;
			break;

			case 0x04:  // PUSH r
				op = PUSH;
				goto just_r;
			case 0x05:  // POP r
				op = POP;
				goto just_r;
			case 0x06:  // CPL r
				op = CPL;
				goto just_r;
			case 0x07:  // NEG r
				op = NEG;
				goto just_r;
			case 0x0d:  // UNLK r
				op = UNLK;
				goto just_r;
			case 0x10:  // DAA r
				op = DAA;
				goto just_r;
			case 0x12:  // EXTZ r
				op = EXTZ;
				goto just_r;
			case 0x13:  // EXTS r
				op = EXTS;
				goto just_r;
			case 0x14:  // PAA r
				op = PAA;
				goto just_r;
			case 0x16:  // MIRR r
				op = MIRR;
				goto just_r;
			case 0x19:  // MULA rr
				op = MULA;
			just_r:
				sprintf(dd->ops,"%s",regs[r]);
				dd->opt = OPT_1_n_1;
			break;

			case 0x1c:  // DJNZ [r],$+3/4+d8
				base = dd->addr + get8(b+len); //dd->base + dd->pos + get8(b+len);
				op = DJNZ;
				dd->opt = OPT_1_n_1_1;

				if (len > 2) {
					// using extended r code..
					base += 4;
				} else {
					base += 3;
				}
				if (r == 2 && zz == 0) {  // i.e. r == B
					sprintf(dd->ops,"%07Xh",base);
				} else {
					sprintf(dd->ops,"%s,%07Xh",regs[r],base);
				}
				len++;
			break;

			case 0x0e:  // BS1F A,r
				op = BS1F;
				goto a_r;
			case 0x0f:  // BS1B A,r
				op = BS1B;
				goto a_r;
			case 0x28:  // ANDCF A,r
				op = ANDCF;
				goto a_r;
			case 0x29:  // ORCF A,r
				op = ORCF;
				goto a_r;
			case 0x2a:  // XORCF A,r
				op = XORCF;
				goto a_r;
			case 0x2b:  // LDCF A,r
				op = LDCF;
				goto a_r;
			case 0x2c:  // STCF A,r
				op = STCF;
				goto a_r;
			case 0xf8:  // RLC A,r
				op = RLC;
				goto a_r;
			case 0xf9:  // RRC A,r
				op = RRC;
				goto a_r;
			case 0xfa:  // RL A,r
				op = RL;
				goto a_r;
			case 0xfb:  // RR A,r
				op = RR;
				goto a_r;
			case 0xfc:  // SLA A,r
				op = SLA;
				goto a_r;
			case 0xfd:  // SRA A,r
				op = SRA;
				goto a_r;
			case 0xfe:  // SLL A,r
				op = SLL;
				goto a_r;
			case 0xff:  // SRL A,r
				op = SRL;
			a_r:
				sprintf(dd->ops,"A,%s",regs[r]);
				dd->opt = OPT_1_n_1;
			break;

			case 0x2e:  // LDC cr,r
				dd->opt = OPT_1_n_1_1;
				op = LDC;
				sprintf(dd->ops,"%s,%s",cr_names[b[len++]],regs[r]);
			break;
			case 0x2f:  // LDC r,cr
				dd->opt = OPT_1_n_1_1;
				op = LDC;
				sprintf(dd->ops,"%s,%s",regs[r],cr_names[b[len++]]);
			break;

			case 0x0c:  // LINK r,d16
				op = LINK;
				sprintf(dd->ops,"%s,%05Xh",regs[r],get16(&b[len]) );
				len += 2;
				dd->opt = OPT_1_n_1_2;
			break;
      
			case 0x38:    // MINC1 #,r
				op = MINC1;
				goto minc_mdec;
			case 0x39:    // MINC2 #,r
				op = MINC2;
				goto minc_mdec;
			case 0x3a:    // MINC4 #,r
				op = MINC4;
				goto minc_mdec;
			case 0x3c:    // MDEC1 #,r
				op = MDEC1;
				goto minc_mdec;
			case 0x3d:    // MDEC2 #,r
				op = MDEC2;
				goto minc_mdec;
			case 0x3e:    // MDEC4 #,r
				op = MDEC4;
			minc_mdec:
				sprintf(dd->ops,"%05Xh,%s",get16u(b+len),regs[r]);
				len += 2;
				dd->opt = OPT_1_n_1_1;
			break;

			default:
				// unknown opcode..
				return 0;
		}
	}

	dd->opf = opcode_names[op];
	return len;
}

int tlcs900hdebugger::decode_zz_R( struct tlcs900d *dd ) {
	unsigned char *b = getCodePtr(dd->addr); //dd->buffer + dd->pos;
	char *a;
	int zz = getzz(b);
	int R = getR(b);
	int w = *b & 0x07;
	int len = 1;
	enum opcodes op = INVALID;

	switch (zz) {
		case 0x00:
			a = "A";
		break;
		case 0x01:
			a = "WA";
		break;
		case 0x02:
		case 0x03:
		default:
			// Not a valid instruction..
			return 0;
	}

	//
	// Following many opcodes:
	//   LDI<W>  [(XDE+),(XhL+)]    83+zz:10
	//   LDI<W>  (XIX+),(XIY+)      85+zz:10
	//   LDIR<W> [(XDE+),(XhL+)]    83+zz:11
	//   LDIR<W> (XIX+),(XIY+)      85+zz:11
	//   LDD<W>  [(XDE-),(XhL-)]    83+zz:12
	//   LDD<W>  (XIX-),(XIY-)      85+zz:12
	//   LDDR<W> [(XDE-),(XhL-)]    83+zz:13
	//   LDDR<W> (XIX-),(XIY-)      85+zz:13

	//   CPI     [A/WA,(R+)]        80+zz+R:14
	//   CPIR    [A/WA,(R+)]        80+zz+R:15
	//   CPD     [A/WA,(R-)]        80+zz+R:16
	//   CPDR    [A/WA,(R-)]        80+zz+R:17
	//
	// Here the zz is actually 0z because none
	// of these instructions use longword size.
	//
	//

	switch (b[len++]) {
		case 0x10:  // LDI
			if ( zz == 1 ) {
				op = LDIW;
			} else {
				op = LDI;
			}
		case 0x11:  // LDIR
			if (op == INVALID)
			{
				if ( zz == 1 ) {
					op = LDIRW;
				} else {
					op = LDIR;
				}
			}
			if (w == 5) {
				strcpy(dd->ops,"(XIX+),(XIY+)");
			} else {
				strcpy(dd->ops,"[(XDE+),(XhL+)]");
			}
		break;
		case 0x12:  // LDD
			if ( zz == 1 ) {
				op = LDDW;
			} else {
				op = LDD;
			}
		case 0x13:  // LDDR
			if (op == INVALID) {
				if ( zz == 1 ) {
					op = LDDR;
				} else {
					op = LDDRW;
				}
			}
			if (w == 5) {
				strcpy(dd->ops,"(XIX-),(XIY-)");
			} else {
				strcpy(dd->ops,"[(XDE-),(XhL-)]");
			}
		break;
		case 0x14:  // CPI [A/WA,(R+)]
			op = CPI;
		case 0x15:  // CPIR [A/WA,(R+)]
			if (op == INVALID) { op = CPIR; }
			sprintf(dd->ops,"%s,(%s+)",a,R32_names[R]);
		break;
		case 0x16:  // CPD [A/WA,(R-)]
			op = CPD;
		case 0x17:  // CPDR [A/WA,(R-)]
			if (op == INVALID) { op = CPDR; }
			sprintf(dd->ops,"%s,(%s-)",a,R32_names[R]);
		break;
		default:
		// ...
		return 0;
	}

	dd->opf = opcode_names[op];
	dd->opt = OPT_1_1_0;
	return len;
}

int tlcs900hdebugger::decode_zz_mem( struct tlcs900d * dd)
{
	unsigned char *b = getCodePtr(dd->addr); //dd->buffer + dd->pos;
	char m[MEM_LEN];
	unsigned char c;
	int len;
	int mem;
	int zz;
	int n;
	int R;
	enum opcodes op = INVALID;
	char **Regs;

	//

	dd->opt = OPT_1_n_1;

	//
	// Following many opcodes:
	//  LD<W> (#16),(mem)   80+zz+mem:19:#16  +

	// regs

	//  LD   R,(mem)        80+zz+mem:20+R  +
	//  ADD  R,(mem)        80+zz+mem:80+R  +
	//  ADC  R,(mem)        80+zz+mem:90+R  +
	//  SUB  R,(mem)        80+zz+mem:a0+R  +
	//  SBC  R,(mem)        80+zz+mem:b0+R  +
	//  CP   R,(mem)        80+zz+mem:f0+R  +
	//  MUL  RR,(mem)       80+zz+mem:40+R  +
	//  MULS RR,(mem)       80+zz+mem:48+R  +
	//  DIV  RR,(mem)       80+zz+mem:50+R  +
	//  DIVS RR,(mem)       80+zz+mem:58+R  +
	//  AND  R,(mem)        80+zz+mem:c0+R  +
	//  OR   R,(mem)        80+zz+mem:e0+R  +
	//  XOR  R,(mem)        80+zz+mem:d0+R  +

	//  EX (mem),R          80+zz+mem:30+R  +
	//  ADD (mem),R         80+zz+mem:88+R  +
	//  ADC (mem),R         80+zz+mem:98+R  +
	//  SUB (mem),R         80+zz+mem:a8+R  +
	//  SBC (mem),R         80+zz+mem:b8+R  +
	//  CP (mem),R          80+zz+mem:f8+R  +
	//  AND (mem),R         80+zz+mem:c8+R  +
	//  OR (mem),R          80+zz+mem:e8+R  +
	//  XOR (mem),R         80+zz+mem:d8+R  +

	// inc/dec 1-8

	//  INC<W> #3,(mem)     80+zz+mem:60+#3 +
	//  DEC<W> #3,(mem)     80+zz+mem:68+#3 +

	// 8 or 16 bits..

	//  ADD<W> (mem),#			80+zz+mem:38:#  +
	//  ADC<W> (mem),#      80+zz+mem:39:#  +
	//  SUB<W> (mem),#      80+zz+mem:3a:#  +
	//  SBC<W> (mem),#      80+zz+mem:3b:#  +
	//  CP<W> (mem),#       80+zz+mem:3f:#  +
	//  AND<W> (mem),#      80+zz+mem:3c:#  +
	//  OR<W> (mem),#     	80+zz+mem:3e:#  +
	//  XOR<W> (mem),#      80+zz+mem:3d:#  +

	// 'Fixed' codes..

	//  PUSH<W> (mem)       80+zz+mem:04    +
	//  RLC<W> (mem)        80+zz+mem:78    +
	//  RRC<W> (mem)        80+zz+mem:79    + 
	//  RL<W> (mem)         80+zz+mem:7a    +
	//  RR<W> (mem)         80+zz+mem:7b    +
	//  SLA<W> (mem)        80+zz+mem:7c    +
	//  SRA<W> (mem)        80+zz+mem:7d    +
	//  SLL<W> (mem)        80+zz+mem:7e    +
	//  SRL<W> (mem)        80+zz+mem:7f    +
	//  RLD [A],(mem)       80+mem:06       +
	//  RRD [A],(mem)       80+mem:07       +
	//

	zz  = getzz(b);
	mem = getmem(b);  

	//

	if ((len = retmem(b,m,mem)) == 0) {
		return 0;
	}

	R = getR(b+len);
	n = get3(b+len); 
	c = b[len++];

	switch (zz) {
		case 0:
			Regs = R8_names;
		break;
		case 1:
			Regs = R16_names;
		break;
		case 2:
			Regs = R32_names;
		break;
	}

	//

	if (c == 0x04 || c == 0x06 || c == 0x07 || c == 0x19 ||
		c >= 0x78 && c < 0x80  || c >= 0x38 && c < 0x40)  {

		switch (c) {
			case 0x38:  // ADD<W> (mem),#
				op = zz == 0 ? ADD : ADDW;
				goto w_mem_nro;
			case 0x39:  // ADC<W> (mem),#
				op = zz == 0 ? ADC : ADCW;
				goto w_mem_nro;
			case 0x3a:  // SUB<W> (mem),#
				op = zz == 0 ? SUB : SUBW;
				goto w_mem_nro;
			case 0x3b:  // SBC<W> (mem),#
				op = zz == 0 ? SBC : SBCW;
				goto w_mem_nro;
			case 0x3c:  // AND<W> (mem),# 
				op = zz == 0 ? AND : ANDW;
				goto w_mem_nro;
			case 0x3d:  // XOR<W> (mem),#
				op = zz == 0 ? XOR : XORW;
				goto w_mem_nro;
			case 0x3e:  // OR<W> (mem),#
				op = zz == 0 ? OR : ORW;
				goto w_mem_nro;
			case 0x3f:  // CP<W> (mem),#
				op = zz == 0 ? CP : CPW;
			w_mem_nro:
				if (zz == 0) {
					sprintf(dd->ops,"%s,%03Xh",m,get8u(b+len));
					len++;
					dd->opt = OPT_1_n_1_1;
				} else if (zz == 1) {
					sprintf(dd->ops,"%s,%05Xh",m,get16u(b+len));
					len += 2;
					dd->opt = OPT_1_n_1_2;
				} else {
					// Got this far but this is not a valid instruction..
					return 0;
				}
			break;

			case 0x19:  // LD<W> (#16),(mem)
				dd->opt = OPT_1_n_1_2;
				op = zz == 0 ? LD : LDW;
				sprintf(dd->ops,"(%04Xh),%s",get16u(b+len),m);
				len += 2;
			break;

			case 0x06:  // RLD [A],(mem)
				op = RLD;
			case 0x07:  // RRD [A],(mem)
				if (op == INVALID) { op = RRD; }
				sprintf(dd->ops,"[A],%s",m);
			break;

			case 0x04:  // PUSH<W> (mem)
				op = zz == 0 ? PUSH : PUSHW;
				goto w_mem;
			case 0x78:  // RLC<W> (mem)
				op = zz == 0 ? RLC : RLCW;
				goto w_mem;
			case 0x79:  // RRC<W> (mem)
				op = zz == 0 ? RRC : RRCW;
				goto w_mem;
			case 0x7a:  // RL<W> (mem)
				op = zz == 0 ? RL : RLW;
				goto w_mem;
			case 0x7b:  // RR<W> (mem)
				op = zz == 0 ? RR : RRW;
				goto w_mem;
			case 0x7c:  // SLA<W> (mem)
				op = zz == 0 ? SLA : SLAW;
				goto w_mem;
			case 0x7d:  // SRA<W> (mem)
				op = zz == 0 ? SRA : SRAW;
				goto w_mem;
			case 0x7e:  // SLL<W> (mem) 
				op = zz == 0 ? SLL : SLLW;
				goto w_mem;
			case 0x7f:  // SRL<W> (mem)
				op = zz == 0 ? SRL : SRLW;
			w_mem:
				sprintf(dd->ops,"%s",m);
			break;
			default:
				return 0;
		}
	} else {
		switch ( c & 0xf8 ) {
			case 0x60:  // INC<W> (mem)
				op = zz == 0 ? INC : INCW;
			case 0x68:  // DEC<W> (mem)
				if (op == INVALID) { op = zz == 0 ? DEC : DECW; }
				if (n == 0) { n = 8; }
				sprintf(dd->ops,"%d,%s",n,m);
			break;

			case 0x20:  // LD R,(mem)
				op = LD;
				goto r_mem;
			case 0x80:  // ADD R,(mem)
				op = ADD;
				goto r_mem;
			case 0x90:  // ADC R,(mem)
				op = ADC;
				goto r_mem;
			case 0xa0:  // SUB R,(mem)
				op = SUB;
				goto r_mem;
			case 0xb0:  // SBC R,(mem)
				op = SBC;
				goto r_mem;
			case 0xf0:  // CP R,(mem)
				op = CP;
				goto r_mem;
			case 0x40:  // MUL RR,(mem)
				op = MUL;
				goto r_mem;
			case 0x48:  // MULS RR,(mem)
				op = MULS;
				goto r_mem;
			case 0x50:  // DIV RR,(mem)
				op = DIV;
				goto r_mem;
			case 0x58:  // DIVS RR,(mem)
				op = DIVS;
				goto r_mem;
			case 0xc0:  // AND R,(mem)
				op = AND;
				goto r_mem;
			case 0xe0:  // OR R,(mem)
				op = OR;
				goto r_mem;
			case 0xd0:  // XOR R,(mem)
				op = XOR;
			r_mem:
				sprintf(dd->ops,"%s,%s",Regs[R],m);
			break;

			case 0x30:  // EX (mem),R
				op = EX;
				goto mem_r;
			case 0x88:  // ADD (mem),R
				op = ADD;
				goto mem_r;
			case 0x98:  // ADC (mem),R
				op = ADC;
				goto mem_r;
			case 0xa8:  // SUB (mem),R
				op = SUB;
				goto mem_r;
			case 0xb8:  // SBC (mem),R
				op = SBC;
				goto mem_r;
			case 0xf8:  // CP (mem),R
				op = CP;
				goto mem_r;
			case 0xc8:  // AND (mem),R
				op = AND;
				goto mem_r;
			case 0xe8:  // OR (mem),R
				op = OR;
				goto mem_r;
			case 0xd8:  // XOR (mem),R
				op = XOR;
			mem_r:
				sprintf(dd->ops,"%s,%s",m,Regs[R]);
			break;
			default:
				return 0;
		}
	}

	//
	dd->opf = opcode_names[op];
	return len;
}

#endif