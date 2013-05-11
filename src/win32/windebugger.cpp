#include "windebugger.h"


// TLCS900-h Debugger
#ifdef TLCS900H_DEBUGGER

// TLCS900h & Z80
#include "../cpu/tlcs900h.h"
#include "../cpu/z80.h"

// Memory for debugging
#include "../core/memory.h"

// Windows include
#include "../win32/resource.h"

HWND tlcs900hHwnd;
HWND tlcs900hList;
HWND z80Hwnd;

void setupTLCS900hDebugger(HWND hWnd)
{
	tlcs900hHwnd = hWnd;

	//LRESULT lResult = SendMessage(hwndList, LB_ADDSTRING, NULL, sound_text);
	tlcs900hList = GetDlgItem(tlcs900hHwnd, IDC_OPLIST);

	if ( tlcs900hList != NULL )
	{
		tlcs900h_debug_decode_rom();
	}
}

void runTLCS900hDebugger()
{
	// Update with the debugger as we run through the PC
}

void shutdownTLCS900hDebugger()
{
	tlcs900hHwnd = NULL;
}

void setupZ80Debugger(HWND hWnd)
{
	z80Hwnd = hWnd;
}

void runZ80Debugger()
{
}

void shutdownZ80Debugger()
{
	z80Hwnd = NULL;
}

// opcodes supported by tlcs900h
enum opcodes {
	LD=0,	LDW,  PUSH,	PUSHW,  POP,	POPW,   LDA,	  LDAR,
	EX,		MIRR, LDI,  LDIW,	  LDIR, LDIRW,	LDD,	  LDDW,
  LDDR, LDDRW,CPI,	CPIR,	  CPD,	CPDR, 	ADD,	  ADDW,
  ADC,	ADCW, SUB,	SUBW,   SBC,	SBCW,   CP,     CPW,
  INC,  INCW,	DEC,  DECW,	  NEG,  EXTZ,	  EXTS,	  DAA,
	PAA,	MUL,	MULS,	DIV,	  DIVS,	MULA,	  MINC1,  MINC2,
  MINC4,MDEC1,MDEC2,MDEC4,	AND,	ADNW,   OR,     ORW,
  XOR,  XORW,	CPL,  LDCF,	  STCF,	ANDCF,	ORCF,	  XORCF,
  RCF,	SCF,	CCF,	ZCF,	  BIT,	RES,	  SET,	  CHG,
  TSET,	BS1F, BS1B,	NOP,	  EI,		DI,		  SWI,	  HALT,
  LDC,	LDX,  LINK,	UNLK,	  LDF,  INCF,   DECF,	  SCC,
  RLC,	RRC,  RL,	  RR,		  SLA,  SRA,    SLL,    SRL,
	RLD,	RRD,  JP,	  JR,	    JRL,  CALL,   CALR,	  DJNZ,
  RET,	RETD, RETI,
  INVALID
};

// output type for opcode
enum output_types {
  OPT_1_0_0=0, OPT_1_1_0, OPT_1_1_1, OPT_1_1_2, OPT_1_2_0, OPT_1_3_0,
  OPT_2_1_2, OPT_2_0_0, OPT_1_n_1, OPT_1_n_1_1, OPT_1_n_1_2,
  OPT_1_n_2, OPT_1_n_1_4, OPT_1_4_0, OPT_1_1_1_1_1_1
};

// helper functions
unsigned char get8u( unsigned char *b ) {
  return *b;
}

unsigned short get16u( unsigned char *b ) {
  return ((b[1] << 8) | *b);
}

unsigned int get24u( unsigned char *b ) {
  return ((b[2] << 16) | (b[1] << 8) | *b);
}

unsigned int get32u( unsigned int *b ) {
  return ((b[3] << 24) | (b[2] << 16) | (b[1] << 8) | *b);
}

// Opcode function defines
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
  "RLC",  "RRC",  "RL",   "RR",     "SLA",	  "SRA",    "SLL",	  "SRL",
  "RLD",	"RRD",  "JP",   "JR",	    "JRL",	  "CALL",   "CALR",	  "DJNZ",
  "RET",	"RETD",   "RETI",
  "INVALID"
};

// Read Byte defines
char *R8_names[] = {
    "W","A","B","C","D","E","H","L",
};

// Read word defines
char *R16_names[] = {
    "WA","BC","DE","HL","IX","IY","IZ","SP",
};

// Read double defines
char *R32_names[] = {
    "XWA","XBC","XDE","XhL","XIX","XIY","XIZ","XSP",
};

// Read bytes
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

// Read words
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

// Read doubles
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

// Flags
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

// Sound registers
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

// Address names (direct access)
char *addr_names[] = {
    "(XWA)","(XBC)","(XDE)","(XhL)","(XIX)","(XIY)","(XIZ)","(XSP)",
    "(XWA+","(XBC+","(XDE+","(XhL+","(XIX+","(XIY+","(XIZ+","(XSP+",
    "","","",
    "","","","","",""
};


// Decode the entire rom
void tlcs900h_debug_decode_rom()
{
	// Start with the beginning of the ROM, go to the end
	unsigned char * b = &memROM[0];		// Memory of ROM (0x0000000-0x4000000)
	char *opf;							// opcode function string
	char ops[80];						// opcode output features
	opcodes op;							// enum opcode
	unsigned int base = 0; //???? 0x000000 ?
	unsigned int pos = 0;  //???? b - memRom[0] ?
	unsigned int d;    // opcode call distance
	int opt;							// output type
	int len;							// length of opcode (does it match TLCS900h emulator?)
	while ( b < &memROM[0x400000] ) // go through the entire ROM
	{
		*ops = '\0';		// null our ops string
		op = INVALID;		// set the opcode to invalid
		opt = OPT_1_0_0;	// set output to 1-0-0

		b = &memROM[pos];

		// Read the opcode
		switch (*b) {
		case 0x08: // LD (#8),#8
			op = LD;
			sprintf(ops,"(%02Xh),%03Xh",get8u(b+1),get8u(b+2) );
			len = 1+2;
			opt = OPT_1_1_1;
		break;
		case 0x0a: // LDW (#8),#16
			op = LDW;
			sprintf(ops,"(%02Xh),%05Xh",get8u(b+1),get16u(b+2) );
			len = 1+3;
			opt = OPT_1_1_2;
		break;
		case 0x09: // PUSH #8
			op = PUSH;
			sprintf(ops,"%03Xh",get8u(b+1) );
			len = 1+1;
			opt = OPT_1_1_0;
		break;
		case 0x0b: // PUSHW #16
			op = PUSHW;
			sprintf(ops,"%05Xh",get16u(b+1) );
			len = 1+2;
			opt = OPT_1_2_0;
		break;
		case 0x18: // PUSH F
			op = PUSH;
			sprintf(ops,"F");
		break;
		case 0x14: // PUSH A
			op = PUSH;
			sprintf(ops,"A");
		break;
		case 0x19: // POP F
			op = POP;
			sprintf(ops,"F");
		break;
		case 0x15: // POP A
			op = POP;
			sprintf(ops,"A");
		break;
		case 0x16: // EX F,F'
			op = EX;
			sprintf(ops,"F,F'");
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
				sprintf(ops,"%d",b[1] & 0x07);
			}
			len = 1+1;
			opt = OPT_1_1_0;
		break;
		case 0x02: // PUSH SR
			op = PUSH;
			sprintf(ops,"SR");
		break;
		case 0x03: // POP SR
			op = POP;
			sprintf(ops,"SR");
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
			sprintf(ops,"%05Xh",get16u(b+1));
			len = 1+2;
			opt = OPT_1_2_0;
		break;
		case 0x1b: // JP #24
			op = JP;
			sprintf(ops,"%07Xh",get24u(b+1));
			len = 1+3;
			opt = OPT_1_3_0;
		break;
		case 0x1c: // CALL #16
			op = CALL;
			sprintf(ops,"%05Xh",get16u(b+1));
			len = 1+2;
			opt = OPT_1_2_0;
		break;
		case 0x1d: // CALL #24
			op = CALL;
			sprintf(ops,"%07Xh",get24u(b+1));
			len = 1+3;
			opt = OPT_1_3_0;
		break;
		case 0x1e: // CALR d16 !!!!
			op = CALR;
			d = base + pos + 3 + get16u(b+1);
			sprintf(ops,"%07Xh",d);   // relative to the address..
			len = 1+2;
			opt = OPT_1_2_0;
		break;
		case 0x0e: // RET
			op = RET;
		break;
		case 0x0f: // RETD d16
			op = RETD;
			sprintf(ops,"%05Xh",get16u(b+1));
			len = 1+2;
			opt = OPT_1_2_0;
		break;
		case 0x07: // RETI
			op = RETI;
		break;
		case 0xf7: // LDX (#8),#8
			op = LDX;
			sprintf(ops,"(%02Xh),%03Xh",get8u(b+2),get8u(b+4));
			len = 1+5;
			opt = OPT_1_1_1_1_1_1;
		break;
		case 0xf8: // SWI [#3];
		case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			op = SWI;
			sprintf(ops,"%d",*b & 0x07);
		break;
		case 0x17: // LDF #3
			op = LDF;
			sprintf(ops,"%d",get8u(b+1) & 0x07);
			len = 1+1;
			opt = OPT_1_1_0;
		break;
		default:
			len = 0;
		break;
		}
		if ( len == 0 )
		{
			// Go onto other tests
			/*
			mem = getmem(b);
			zz = getzz(b);

			if (*b >= 0x80) {
				if (zz == 0x03) {
					//
					// These are definitely instructions that include
					// mem (-x--xxxx) part in the opcode.. or then LADR..
					//
					// Opcodes are marked as: B0+mem
					//

					return decode_B0_mem(dd);
				} else {
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
						return decode_zz_r(dd);
					} else if (mem <= 0x07 && b[1] >= 0x10 && b[1] <= 0x17) {
						return decode_zz_R(dd);
					} else if (mem <= 0x15) {
						return decode_zz_mem(dd);
					}
				}
			} else {
				return decode_xx(dd);
			}
			*/
			//printf("%02X                ?????\n",dd->buffer[dd->pos++]);
			pos++; // increase our position by one (we don't know what this is?)
		}
		else
		{
			// Print it out
			opf = opcode_names[op];
			pos += len;
		}
	}
}

#endif

