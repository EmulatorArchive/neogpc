#include "tlcs900hdebugger.h"

// Do not include code if we are not using the debugger
#ifdef NEOGPC_DEBUGGER

#include <string.h>
#include <stdio.h>
#include <algorithm> // sorting

// ROM, RAM, Bios
#include "../core/memory.h"

// How many bytes will each function need
enum {
	NONE = 0,			// pushA
	ONE_BYTE,			// ei #$1
	TWO_BYTES,			// call16 #$2001
	THREE_BYTES,		// jp24 #$200101
	FOUR_BYTES,			// ldRIL #$04040404
	DECODE_NONE,		// decodeC1,D1,E1
	DECODE_ONE_BYTE,	// decodeC8,D8,E8
	DECODE_N_BYTES,		// decodeC3
	NONE_THREE_MEM,		// 10111mmm
	NONE_THREE_REG,		// 10001rrr
	NONE_THREE_BITS		// SWI #5 11111xxx
};

// All instruction names for standard opcodes
char *instr_names[256] =
{
    "nop", "normal", "pushsr", "popsr", "tmax", "halt", "ei", "reti",
    "ld8I", "pushI", "ldw8I", "pushwI", "incf", "decf", "ret", "retd",
    "rcf", "scf", "ccf", "zcf", "pushA", "popA", "exFF", "ldf",
    "pushF", "popF", "jp16", "jp24", "call16", "call24", "calr", "udef",
    "ldRIB", "ldRIB", "ldRIB", "ldRIB", "ldRIB", "ldRIB", "ldRIB", "ldRIB",
    "pushRW", "pushRW", "pushRW", "pushRW", "pushRW", "pushRW", "pushRW", "pushRW",
    "ldRIW", "ldRIW", "ldRIW", "ldRIW", "ldRIW", "ldRIW", "ldRIW", "ldRIW",
    "pushRL", "pushRL", "pushRL", "pushRL", "pushRL", "pushRL", "pushRL", "pushRL",
    //
    "ldRIL", "ldRIL", "ldRIL", "ldRIL", "ldRIL", "ldRIL", "ldRIL", "ldRIL",
    "popRW", "popRW", "popRW", "popRW", "popRW", "popRW", "popRW", "popRW",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "popRL", "popRL", "popRL", "popRL", "popRL", "popRL", "popRL", "popRL",
    "jrcc0", "jrcc1", "jrcc2", "jrcc3", "jrcc4", "jrcc5", "jrcc6", "jrcc7",
    "jrcc8", "jrcc9", "jrccA", "jrccB", "jrccC", "jrccD", "jrccE", "jrccF",
    "jrlcc0", "jrlcc1", "jrlcc2", "jrlcc3", "jrlcc4", "jrlcc5", "jrlcc6", "jrlcc7",
    "jrlcc8", "jrlcc9", "jrlccA", "jrlccB", "jrlccC", "jrlccD", "jrlccE", "jrlccF",
    //
    "decode80", "decode80", "decode80", "decode80", "decode80", "decode80", "decode80", "decode80",
    "decode88", "decode88", "decode88", "decode88", "decode88", "decode88", "decode88", "decode88",
    "decode90", "decode90", "decode90", "decode90", "decode90", "decode90", "decode90", "decode90",
    "decode98", "decode98", "decode98", "decode98", "decode98", "decode98", "decode98", "decode98",
    "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0",
    "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8",
    "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0",
    "decodeB8", "decodeB8", "decodeB8", "decodeBB", "decodeB8", "decodeB8", "decodeB8", "decodeB8",
    //
    "decodeC0", "decodeC1", "decodeC2", "decodeC3", "decodeC4", "decodeC5", "udef", "decodeC7",
    "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8",
    "decodeD0", "decodeD1", "decodeD2", "decodeD3", "decodeD4", "decodeD5", "udef", "decodeD7",
    "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8",
    "decodeE0", "decodeE1", "decodeE2", "decodeE3", "decodeE4", "decodeE5", "udef", "decodeE7",
    "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8",
    "decodeF0", "decodeF1", "decodeF2", "decodeF3", "decodeF4", "decodeF5", "udef", "ldx",
    "swi", "swi", "swi", "swi", "swi", "swi", "swi", "swi"
};

// How many bytes does each opcode read
int instr_names_readbytes[256] =
{
    NONE, NONE, NONE, NONE, NONE, NONE, ONE_BYTE, NONE,
    TWO_BYTES, ONE_BYTE, THREE_BYTES, TWO_BYTES, NONE, NONE, NONE, TWO_BYTES,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, ONE_BYTE,
    NONE, NONE, TWO_BYTES, THREE_BYTES, TWO_BYTES, THREE_BYTES, TWO_BYTES, NONE,
    ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
    TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
    //
    FOUR_BYTES, FOUR_BYTES, FOUR_BYTES, FOUR_BYTES, FOUR_BYTES, FOUR_BYTES, FOUR_BYTES, FOUR_BYTES,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
    ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE,
    ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE, ONE_BYTE,
    TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES,
    TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES, TWO_BYTES,
    
	// Decode80-DecodeB8
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE,
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE,
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE,
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_NONE,

    // decode has extra bytes (decodeC3,decodeD3,decodeE3)
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_ONE_BYTE, DECODE_NONE, DECODE_NONE, NONE, DECODE_NONE, // decodeC3
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_ONE_BYTE, DECODE_NONE, DECODE_NONE, NONE, DECODE_NONE, // decodeD3
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_ONE_BYTE, DECODE_NONE, DECODE_NONE, NONE, DECODE_NONE, // decodeE3
    DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE, DECODE_ONE_BYTE,
    DECODE_NONE, DECODE_NONE, DECODE_NONE, DECODE_ONE_BYTE, DECODE_NONE, DECODE_NONE, NONE, TWO_BYTES,
    NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS, NONE_THREE_BITS
};

/*
int instr_names_timing[256] =
{
    1, 4, 4, 6, 4, 8, 5, 12,
    5, 4, 6, 5, 2, 2, 9, 9,
    2, 2, 2, 2, 3, 4, 2, 2,	
    3, 4, 7, 7, 12, 12, 12, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    5, 5, 5, 5, 5, 5, 5, 5,
    //
    5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4,
    1, 1, 1, 1, 1, 1, 1, 1,
    6, 6, 6, 6, 6, 6, 6, 6,
    4, 4/8, "jrcc2", "jrcc3", "jrcc4", "jrcc5", "jrcc6", "jrcc7",
    "jrcc8", "jrcc9", "jrccA", "jrccB", "jrccC", "jrccD", "jrccE", "jrccF",
    "jrlcc0", "jrlcc1", "jrlcc2", "jrlcc3", "jrlcc4", "jrlcc5", "jrlcc6", "jrlcc7",
    "jrlcc8", "jrlcc9", "jrlccA", "jrlccB", "jrlccC", "jrlccD", "jrlccE", "jrlccF",
    //
    "decode80", "decode80", "decode80", "decode80", "decode80", "decode80", "decode80", "decode80",
    "decode88", "decode88", "decode88", "decode88", "decode88", "decode88", "decode88", "decode88",
    "decode90", "decode90", "decode90", "decode90", "decode90", "decode90", "decode90", "decode90",
    "decode98", "decode98", "decode98", "decode98", "decode98", "decode98", "decode98", "decode98",
    "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0", "decodeA0",
    "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8", "decodeA8",
    "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0", "decodeB0",
    "decodeB8", "decodeB8", "decodeB8", "decodeBB", "decodeB8", "decodeB8", "decodeB8", "decodeB8",
    //
    "decodeC0", "decodeC1", "decodeC2", "decodeC3", "decodeC4", "decodeC5", "udef", "decodeC7",
    "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8", "decodeC8",
    "decodeD0", "decodeD1", "decodeD2", "decodeD3", "decodeD4", "decodeD5", "udef", "decodeD7",
    "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8", "decodeD8",
    "decodeE0", "decodeE1", "decodeE2", "decodeE3", "decodeE4", "decodeE5", "udef", "decodeE7",
    "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8", "decodeE8",
    "decodeF0", "decodeF1", "decodeF2", "decodeF3", "decodeF4", "decodeF5", "udef", "ldx",
    16, 16, 16, 16, 16, 16, 16, 16
};
*/

// Decode80 opcodes
char *instr_table80[256] =
{
    "udef", "udef", "udef", "udef", "pushM00", "udef", "rld00", "rrd00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldi", "ldir", "ldd", "lddr", "cpiB", "cpirB", "cpdB", "cpdrB",
    "udef", "ld16M00", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00",
    "addMI00", "adcMI00", "subMI00", "sbcMI00", "andMI00", "xorMI00", "orMI00", "cpMI00",
    //
    "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00",
    "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00",
    "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00",
    "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00",
    "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00",
    "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "rlcM00", "rrcM00", "rlM00", "rrM00", "slaM00", "sraM00", "sllM00", "srlM00",
    //
    "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00",
    "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00",
    "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00",
    "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00",
    "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00",
    "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00",
    "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00",
    "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00",
    //
    "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00",
    "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00",
    "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00",
    "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00",
    "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00",
    "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00",
    "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00",
    "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00"
};

// Decode90 Opcodes
char *instr_table90[256] =
{
    "udef", "udef", "udef", "udef", "pushwM10", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldiw", "ldirw", "lddw", "lddrw", "cpiW", "cpirW", "cpdW", "cpdrW",
    "udef", "ldw16M10", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10",
    "addwMI10", "adcwMI10", "subwMI10", "sbcwMI10", "andwMI10", "xorwMI10", "orwMI10", "cpwMI10",
    //
    "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10",
    "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10",
    "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10",
    "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10",
    "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10",
    "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "rlcwM10", "rrcwM10", "rlwM10", "rrwM10", "slawM10", "srawM10", "sllwM10", "srlwM10",
    //
    "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10",
    "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10",
    "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10",
    "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10",
    "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10",
    "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10",
    "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10",
    "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10",
    //
    "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10",
    "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10",
    "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10",
    "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10",
    "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10",
    "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10",
    "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10",
    "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10"
};

// Decode98 Opcodes
char *instr_table98[256] =
{
    "udef", "udef", "udef", "udef", "pushwM10", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "ldw16M10", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10",
    "addwMI10", "adcwMI10", "subwMI10", "sbcwMI10", "andwMI10", "xorwMI10", "orwMI10", "cpwMI10",
    //
    "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10",
    "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10",
    "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10",
    "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10",
    "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10",
    "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "rlcwM10", "rrcwM10", "rlwM10", "rrwM10", "slawM10", "srawM10", "sllwM10", "srlwM10",
    //
    "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10",
    "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10",
    "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10",
    "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10",
    "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10",
    "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10",
    "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10",
    "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10",
    //
    "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10",
    "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10",
    "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10",
    "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10",
    "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10",
    "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10",
    "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10",
    "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10"
};

// DecodeA0 opcodes
char *instr_tableA0[256] =
{
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef ", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20",
    "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20",
    "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20",
    "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20",
    "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20",
    "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20",
    "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20",
    "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20",
    //
    "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20",
    "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20",
    "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20",
    "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20",
    "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20",
    "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20",
    "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20",
    "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20"
};

// DecodeB0 opcodes
char *instr_tableB0[256] =
{
    "ldMI30", "udef", "ldwMI30", "udef", "popM30", "udef", "popwM30", "udef",
    "udef", "udef", "udef", "udef", "udef ", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "ldM1630", "udef", "ldwM1630", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30",
    "andcfAM30", "orcfAM30", "xorcfAM30", "ldcfAM30", "stcfAM30", "udef", "udef", "udef",
    "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W",
    "udef", "udef", "udef ", "udef", "udef", "udef", "udef", "udef",
    "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef ", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30",
    "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30",
    "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30",
    "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30",
    "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30",
    "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30",
    "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30",
    "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30",
    //
    "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30",
    "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30",
    "jpccM300", "jpccM301", "jpccM302", "jpccM303", "jpccM304", "jpccM305", "jpccM306", "jpccM307",
    "jpccM308", "jpccM309", "jpccM30A", "jpccM30B", "jpccM30C", "jpccM30D", "jpccM30E", "jpccM30F",
    "callccM300", "callccM301", "callccM302", "callccM303", "callccM304", "callccM305", "callccM306", "callccM307",
    "callccM308", "callccM309", "callccM30A", "callccM30B", "callccM30C", "callccM30D", "callccM30E", "callccM30F",
    "retcc0", "retcc1", "retcc2", "retcc3", "retcc4", "retcc5", "retcc6", "retcc7",
    "retcc8", "retcc9", "retccA", "retccB", "retccC", "retccD", "retccE", "retccF"
};

// DecodeB8 opcodes
char *instr_tableB8[256] =
{
    "ldMI30", "udef", "ldwMI30", "udef", "popM30", "udef", "popwM30", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "ldM1630", "udef", "ldwM1630", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30",
    "andcfAM30", "orcfAM30", "xorcfAM30", "ldcfAM30", "stcfAM30", "udef", "udef", "udef",
    "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30",
    "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30",
    "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30",
    "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30",
    "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30",
    "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30",
    "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30",
    "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30",
    //
    "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30",
    "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30",
    "jpccM300", "jpccM301", "jpccM302", "jpccM303", "jpccM304", "jpccM305", "jpccM306", "jpccM307",
    "jpccM308", "jpccM309", "jpccM30A", "jpccM30B", "jpccM30C", "jpccM30D", "jpccM30E", "jpccM30F",
    "callccM300", "callccM301", "callccM302", "callccM303", "callccM304", "callccM305", "callccM306", "callccM307",
    "callccM308", "callccM309", "callccM30A", "callccM30B", "callccM30C", "callccM30D", "callccM30E", "callccM30F",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef"
};

// DecodeC0 opcodes
char *instr_tableC0[256] =
{
    "udef", "udef", "udef", "udef", "pushM00", "udef", "rld00", "rrd00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "ld16M00", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00", "ldRM00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00", "exMRB00",
    "addMI00", "adcMI00", "subMI00", "sbcMI00", "andMI00", "xorMI00", "orMI00", "cpMI00",
    //
    "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00", "mulRMB00",
    "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00", "mulsRMB00",
    "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00", "divRMB00",
    "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00", "divsRMB00",
    "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00", "inc3M00",
    "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00", "dec3M00",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "rlcM00", "rrcM00", "rlM00", "rrM00", "slaM00", "sraM00", "sllM00", "srlM00",
    //
    "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00", "addRMB00",
    "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00", "addMRB00",
    "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00", "adcRMB00",
    "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00", "adcMRB00",
    "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00", "subRMB00",
    "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00", "subMRB00",
    "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00", "sbcRMB00",
    "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00", "sbcMRB00",
    //
    "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00", "andRMB00",
    "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00", "andMRB00",
    "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00", "xorRMB00",
    "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00", "xorMRB00",
    "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00", "orRMB00",
    "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00", "orMRB00",
    "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00", "cpRMB00",
    "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00", "cpMRB00"
};

// DecodeC8 opcodes
char *instr_tableC8[256] =
{
    "udef", "udef", "udef", "ldrIB", "pushrB", "poprB", "cplrB", "negrB",
    "mulrIB", "mulsrIB", "divrIB", "divsrIB", "udef", "udef", "udef", "udef",
    "daar", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "bios", "udef", "djnzB", "udef", "udef", "udef",
    "andcf4rB", "orcf4rB", "xorcf4rB", "ldcf4rB", "stcf4rB", "udef", "udef", "udef",
    "andcfArB", "orcfArB", "xorcfArB", "ldcfArB", "stcfArB", "udef", "ldccrB", "ldcrcB",
    "res4rB", "set4rB", "chg4rB", "bit4rB", "tset4rB", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "mulRrB", "mulRrB", "mulRrB", "mulRrB", "mulRrB", "mulRrB", "mulRrB", "mulRrB",
    "mulsRrB", "mulsRrB", "mulsRrB", "mulsRrB", "mulsRrB", "mulsRrB", "mulsRrB", "mulsRrB",
    "divRrB", "divRrB", "divRrB", "divRrB", "divRrB", "divRrB", "divRrB", "divRrB",
    "divsRrB", "divsRrB", "divsRrB", "divsRrB", "divsRrB", "divsRrB", "divsRrB", "divsRrB",
    "inc3rB", "inc3rB", "inc3rB", "inc3rB", "inc3rB", "inc3rB", "inc3rB", "inc3rB",
    "dec3rB", "dec3rB", "dec3rB", "dec3rB", "dec3rB", "dec3rB", "dec3rB", "dec3rB",
    "sccB0", "sccB1", "sccB2", "sccB3", "sccB4", "sccB5", "sccB6", "sccB7",
    "sccB8", "sccB9", "sccBA", "sccBB", "sccBC", "sccBD", "sccBE", "sccBF",
    //
    "addRrB", "addRrB", "addRrB", "addRrB", "addRrB", "addRrB", "addRrB", "addRrB",
    "ldRrB", "ldRrB", "ldRrB", "ldRrB", "ldRrB", "ldRrB", "ldRrB", "ldRrB",
    "adcRrB", "adcRrB", "adcRrB", "adcRrB", "adcRrB", "adcRrB", "adcRrB", "adcRrB",
    "ldrRB", "ldrRB", "ldrRB", "ldrRB", "ldrRB", "ldrRB", "ldrRB", "ldrRB",
    "subRrB", "subRrB", "subRrB", "subRrB", "subRrB", "subRrB", "subRrB", "subRrB",
    "ldr3B", "ldr3B", "ldr3B", "ldr3B", "ldr3B", "ldr3B", "ldr3B", "ldr3B",
    "sbcRrB", "sbcRrB", "sbcRrB", "sbcRrB", "sbcRrB", "sbcRrB", "sbcRrB", "sbcRrB",
    "exRrB", "exRrB", "exRrB", "exRrB", "exRrB", "exRrB", "exRrB", "exRrB",
    //
    "andRrB", "andRrB", "andRrB", "andRrB", "andRrB", "andRrB", "andRrB", "andRrB",
    "addrIB", "adcrIB", "subrIB", "sbcrIB", "andrIB", "xorrIB", "orrIB", "cprIB",
    "xorRrB", "xorRrB", "xorRrB", "xorRrB", "xorRrB", "xorRrB", "xorRrB", "xorRrB",
    "cpr3B", "cpr3B", "cpr3B", "cpr3B", "cpr3B", "cpr3B", "cpr3B", "cpr3B",
    "orRrB", "orRrB", "orRrB", "orRrB", "orRrB", "orRrB", "orRrB", "orRrB",
    "rlc4rB", "rrc4rB", "rl4rB", "rr4rB", "sla4rB", "sra4rB", "sll4rB", "srl4rB",
    "cpRrB", "cpRrB", "cpRrB", "cpRrB", "cpRrB", "cpRrB", "cpRrB", "cpRrB",
    "rlcArB", "rrcArB", "rlArB", "rrArB", "slaArB", "sraArB", "sllArB", "srlArB"
};

// DecodeD0 opcodes
char *instr_tableD0[256] =
{
    "udef", "udef", "udef", "udef", "pushwM10", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "ldw16M10", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10", "ldRM10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10", "exMRW10",
    "addwMI10", "adcwMI10", "subwMI10", "sbcwMI10", "andwMI10", "xorwMI10", "orwMI10", "cpwMI10",
    //
    "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10", "mulRMW10",
    "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10", "mulsRMW10",
    "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10", "divRMW10",
    "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10", "divsRMW10",
    "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10", "incw3M10",
    "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10", "decw3M10",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "rlcwM10", "rrcwM10", "rlwM10", "rrwM10", "slawM10", "srawM10", "sllwM10", "srlwM10",
    //
    "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10", "addRMW10",
    "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10", "addMRW10",
    "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10", "adcRMW10",
    "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10", "adcMRW10",
    "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10", "subRMW10",
    "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10", "subMRW10",
    "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10", "sbcRMW10",
    "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10", "sbcMRW10",
    //
    "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10", "andRMW10",
    "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10", "andMRW10",
    "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10", "xorRMW10",
    "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10", "xorMRW10",
    "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10", "orRMW10",
    "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10", "orMRW10",
    "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10", "cpRMW10",
    "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10", "cpMRW10"
};

// DecodeD8 opcodes
char *instr_tableD8[256] =
{
    "udef", "udef", "udef", "ldrIW", "pushrW", "poprW", "cplrW", "negrW",
    "mulrIW", "mulsrIW", "divrIW", "divsrIW", "udef", "udef", "bs1f", "bs1b",
    "udef", "udef", "extzrW", "extsrW", "paarW", "udef", "mirrr", "udef",
    "udef", "mular", "udef", "udef", "djnzW", "udef", "udef", "udef",
    "andcf4rW", "orcf4rW", "xorcf4rW", "ldcf4rW", "stcf4rW", "udef", "udef", "udef",
    "andcfArW", "orcfArW", "xorcfArW", "ldcfArW", "stcfArW", "udef", "ldccrW", "ldcrcW",
    "res4rW", "set4rW", "chg4rW", "bit4rW", "tset4rW", "udef", "udef", "udef",
    "minc1", "minc2", "minc4", "udef", "mdec1", "mdec2", "mdec4", "udef",
    //
    "mulRrW", "mulRrW", "mulRrW", "mulRrW", "mulRrW", "mulRrW", "mulRrW", "mulRrW",
    "mulsRrW", "mulsRrW", "mulsRrW", "mulsRrW", "mulsRrW", "mulsRrW", "mulsRrW", "mulsRrW",
    "divRrW", "divRrW", "divRrW", "divRrW", "divRrW", "divRrW", "divRrW", "divRrW",
    "divsRrW", "divsRrW", "divsRrW", "divsRrW", "divsRrW", "divsRrW", "divsRrW", "divsRrW",
    "inc3rW", "inc3rW", "inc3rW", "inc3rW", "inc3rW", "inc3rW", "inc3rW", "inc3rW",
    "dec3rW", "dec3rW", "dec3rW", "dec3rW", "dec3rW", "dec3rW", "dec3rW", "dec3rW",
    "sccW0", "sccW1", "sccW2", "sccW3", "sccW4", "sccW5", "sccW6", "sccW7",
    "sccW8", "sccW9", "sccWA", "sccWB", "sccWC", "sccWD", "sccWE", "sccWF",
    //
    "addRrW", "addRrW", "addRrW", "addRrW", "addRrW", "addRrW", "addRrW", "addRrW",
    "ldRrW", "ldRrW", "ldRrW", "ldRrW", "ldRrW", "ldRrW", "ldRrW", "ldRrW",
    "adcRrW", "adcRrW", "adcRrW", "adcRrW", "adcRrW", "adcRrW", "adcRrW", "adcRrW",
    "ldrRW", "ldrRW", "ldrRW", "ldrRW", "ldrRW", "ldrRW", "ldrRW", "ldrRW",
    "subRrW", "subRrW", "subRrW", "subRrW", "subRrW", "subRrW", "subRrW", "subRrW",
    "ldr3W", "ldr3W", "ldr3W", "ldr3W", "ldr3W", "ldr3W", "ldr3W", "ldr3W",
    "sbcRrW", "sbcRrW", "sbcRrW", "sbcRrW", "sbcRrW", "sbcRrW", "sbcRrW", "sbcRrW",
    "exRrW", "exRrW", "exRrW", "exRrW", "exRrW", "exRrW", "exRrW", "exRrW",
    //
    "andRrW", "andRrW", "andRrW", "andRrW", "andRrW", "andRrW", "andRrW", "andRrW",
    "addrIW", "adcrIW", "subrIW", "sbcrIW", "andrIW", "xorrIW", "orrIW", "cprIW",
    "xorRrW", "xorRrW", "xorRrW", "xorRrW", "xorRrW", "xorRrW", "xorRrW", "xorRrW",
    "cpr3W", "cpr3W", "cpr3W", "cpr3W", "cpr3W", "cpr3W", "cpr3W", "cpr3W",
    "orRrW", "orRrW", "orRrW", "orRrW", "orRrW", "orRrW", "orRrW", "orRrW",
    "rlc4rW", "rrc4rW", "rl4rW", "rr4rW", "sla4rW", "sra4rW", "sll4rW", "srl4rW",
    "cpRrW", "cpRrW", "cpRrW", "cpRrW", "cpRrW", "cpRrW", "cpRrW", "cpRrW",
    "rlcArW", "rrcArW", "rlArW", "rrArW", "slaArW", "sraArW", "sllArW", "srlArW"
};

// DecodeE0 opcodes
char *instr_tableE0[256] =
{
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20", "ldRM20",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef ", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20", "addRML20",
    "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20", "addMRL20",
    "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20", "adcRML20",
    "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20", "adcMRL20",
    "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20", "subRML20",
    "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20", "subMRL20",
    "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20", "sbcRML20",
    "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20", "sbcMRL20",
    //
    "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20", "andRML20",
    "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20", "andMRL20",
    "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20", "xorRML20",
    "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20", "xorMRL20",
    "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20", "orRML20",
    "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20", "orMRL20",
    "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20", "cpRML20",
    "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20", "cpMRL20"
};

// DecodeE8 opcodes
char *instr_tableE8[256] =
{
    "udef", "udef", "udef", "ldrIL", "pushrL", "poprL", "udef", "udef",
    "udef", "udef", "udef", "udef", "link", "unlk", "udef", "udef",
    "udef", "udef", "extzrL", "extsrL", "paarL", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "ldccrL", "ldcrcL",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "inc3rL", "inc3rL", "inc3rL", "inc3rL", "inc3rL", "inc3rL", "inc3rL", "inc3rL",
    "dec3rL", "dec3rL", "dec3rL", "dec3rL", "dec3rL", "dec3rL", "dec3rL", "dec3rL",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "addRrL", "addRrL", "addRrL", "addRrL", "addRrL", "addRrL", "addRrL", "addRrL",
    "ldRrL", "ldRrL", "ldRrL", "ldRrL", "ldRrL", "ldRrL", "ldRrL", "ldRrL",
    "adcRrL", "adcRrL", "adcRrL", "adcRrL", "adcRrL", "adcRrL", "adcRrL", "adcRrL",
    "ldrRL", "ldrRL", "ldrRL", "ldrRL", "ldrRL", "ldrRL", "ldrRL", "ldrRL",
    "subRrL", "subRrL", "subRrL", "subRrL", "subRrL", "subRrL", "subRrL", "subRrL",
    "ldr3L", "ldr3L", "ldr3L", "ldr3L", "ldr3L", "ldr3L", "ldr3L", "ldr3L",
    "sbcRrL", "sbcRrL", "sbcRrL", "sbcRrL", "sbcRrL", "sbcRrL", "sbcRrL", "sbcRrL",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "andRrL", "andRrL", "andRrL", "andRrL", "andRrL", "andRrL", "andRrL", "andRrL",
    "addrIL", "adcrIL", "subrIL", "sbcrIL", "andrIL", "xorrIL", "orrIL", "cprIL",
    "xorRrL", "xorRrL", "xorRrL", "xorRrL", "xorRrL", "xorRrL", "xorRrL", "xorRrL",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "orRrL", "orRrL", "orRrL", "orRrL", "orRrL", "orRrL", "orRrL", "orRrL",
    "rlc4rL", "rrc4rL", "rl4rL", "rr4rL", "sla4rL", "sra4rL", "sll4rL", "srl4rL",
    "cpRrL", "cpRrL", "cpRrL", "cpRrL", "cpRrL", "cpRrL", "cpRrL", "cpRrL",
    "rlcArL", "rrcArL", "rlArL", "rrArL", "slaArL", "sraArL", "sllArL", "srlArL"
};

// DecodeF0 opcodes
char *instr_tableF0[256] =
{
    "ldMI30", "udef", "ldwMI30", "udef", "popM30", "udef", "popwM30", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "ldM1630", "udef", "ldwM1630", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30", "ldaRMW30",
    "andcfAM30", "orcfAM30", "xorcfAM30", "ldcfAM30", "stcfAM30", "udef", "udef", "udef",
    "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30", "ldaRML30",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B", "ldMR30B",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W", "ldMR30W",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L", "ldMR30L",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    //
    "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30", "andcf3M30",
    "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30", "orcf3M30",
    "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30", "xorcf3M30",
    "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30", "ldcf3M30",
    "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30", "stcf3M30",
    "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30", "tset3M30",
    "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30", "res3M30",
    "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30", "set3M30",
    //
    "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30", "chg3M30",
    "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30", "bit3M30",
    "jpccM300", "jpccM301", "jpccM302", "jpccM303", "jpccM304", "jpccM305", "jpccM306", "jpccM307",
    "jpccM308", "jpccM309", "jpccM30A", "jpccM30B", "jpccM30C", "jpccM30D", "jpccM30E", "jpccM30F",
    "callccM300", "callccM301", "callccM302", "callccM303", "callccM304", "callccM305", "callccM306", "callccM307",
    "callccM308", "callccM309", "callccM30A", "callccM30B", "callccM30C", "callccM30D", "callccM30E", "callccM30F",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef",
    "udef", "udef", "udef", "udef", "udef", "udef", "udef", "udef"
};

// Constructor
tlcs900hdebugger::tlcs900hdebugger(void)
{
	m_debugState = DEBUGGER_IDLE;
	for ( int i = 0; i < 100; i++)
	{
		m_breakpointList[i].active = false; // set all breakpoints to nothing
		m_breakpointList[i].buf[0] = 0;
	}
}

// Destructor
tlcs900hdebugger::~tlcs900hdebugger(void)
{
	// Clear all of our string memory
	for (std::map<unsigned long, char*>::iterator it = m_decodeList.begin(); it != m_decodeList.end(); it++)
	{
		delete (*it).second;
	}
}

// Get the memory to the breakpoint list
tlcs900hBreakpoint * tlcs900hdebugger::getBreakpointList()
{
	return &(m_breakpointList[0]);
}

// Set our breakpoint
int tlcs900hdebugger::setBreakpoint(unsigned int address)
{
	for ( int i = 0; i < 100; i++)
	{
		if ( m_breakpointList[i].active == false )
		{
			m_breakpointList[i].address = address;
			m_breakpointList[i].active = true;
			return i; // return the breakpoint we set
		}
	}
	return -1; // nothing found
}

// Set Breakpoint name
void tlcs900hdebugger::setBreakpointName(const unsigned int idx, const char * name)
{
	if ( idx >= 0 && idx < 100 )
	{
		strcpy(m_breakpointList[idx].buf, name);
	}
}

char * tlcs900hdebugger::getBreakpointName(const unsigned int idx)
{
	return m_breakpointList[idx].buf;
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

// pause the running tlcs900h emulation
void tlcs900hdebugger::pause()
{
	m_debugState = DEBUGGER_PAUSE;
}

// step to the next opcode
void tlcs900hdebugger::step()
{
	m_debugState = DEBUGGER_STEP;
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
unsigned int tlcs900hdebugger::decodeTlcs900h(unsigned long addr)	// decode the current PC
{
	// Have we already decoded this instruction?
	//std::map<unsigned long, char *>::iterator it = m_decodeList.find(addr);
	//if ( it != m_decodeList.end() )
	//{
	//	return m_decodeList;
	//}
	
	// How many bytes did we eat?
	int bytesRead = 0;

	// Create our instruction
	char * instrBuf = new char[128];

	unsigned char * readBuf = getCodePtr(addr);
	unsigned char opcode = readBuf[0];
	
	// Potential read bytes
	unsigned char readByte[4];		// 8 bits [x4.. up to 32 bits]

	unsigned int opcodeType = instr_names_readbytes[readBuf[0]];
	
	switch(opcodeType) {
	case NONE:
		instrBuf = new char[128];
		sprintf(instrBuf, "0x%06x: %s", addr, instr_names[opcode]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 1;
	break;
	case ONE_BYTE:
		readByte[0] = readBuf[1];	// read 1 byte
		sprintf(instrBuf, "0x%06x: %s $%02x", addr, instr_names[opcode], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 2;
	break;
	case TWO_BYTES:
		readByte[0] = readBuf[1];
		readByte[1] = readBuf[2];
		sprintf(instrBuf, "0x%06x: %s $%02x%02x", addr, instr_names[opcode], readByte[1], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 3;
	break;
	case THREE_BYTES:
		readByte[0] = readBuf[1];
		readByte[1] = readBuf[2];
		readByte[2] = readBuf[3];
		sprintf(instrBuf, "0x%06x: %s $%02x%02x%02x", addr, instr_names[opcode], readByte[2], readByte[1], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 4;
	break;
	case FOUR_BYTES:
		readByte[0] = readBuf[1];
		readByte[1] = readBuf[2];
		readByte[2] = readBuf[3];
		readByte[3] = readBuf[4];
		sprintf(instrBuf, "0x%06x: %s $%02x%02x%02x%02x", addr, instr_names[opcode], readByte[3], readByte[2], readByte[1], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 5;
	break;
	// TODO: Decodes need to dive deeper!
	case DECODE_NONE:
		sprintf(instrBuf, "0x%06x: %s", addr, instr_names[opcode]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 1;
	break;
	case DECODE_ONE_BYTE:
		readByte[0] = readBuf[1];
		sprintf(instrBuf, "0x%06x: %s $%02x", addr, instr_names[opcode], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 2;
	break;
	case DECODE_N_BYTES:
		readByte[0] = readBuf[1];
		sprintf(instrBuf, "0x%06x: %s $%02x", addr, instr_names[opcode], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 2;
	break;
	//case NONE_THREE_MEM,		// 10111mmm
	//case NONE_THREE_REG,		// 10001rrr
	case NONE_THREE_BITS:
		readByte[0] = (opcode & 3); // 10001xxx
		sprintf(instrBuf, "0x%06x: %s #$%02x", addr, instr_names[opcode], readByte[0]);
		m_decodeList[addr] = instrBuf;
		bytesRead = 1;
	break;
	default:
		sprintf(instrBuf, "0x%06x: INVALID", addr);
		m_decodeList[addr] = instrBuf;
		bytesRead = 1; // default unknown to 1
		break;
	};

	return bytesRead;
}

#endif