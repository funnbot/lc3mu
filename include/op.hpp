#ifndef ENUMS_INCLUDE
#define ENUMS_INCLUDE

#include <string>

// 0000_0000_0000_0000
// NAME	Bits F-C	Bits B-0
// ADD	0001	    DR#|SR1|0|00|SR2
// ADD	0001	    DR#|SR1|1| imm5
// AND	0101	    DR#|SR1|0|00|SR2
// AND	0101	    DR#|SR1|1| imm5
// NOT	1001	    DR#|SR1| 111111
// BR	0000	    NZP| PC Offset 9  (ADD, AND, NOT, LD, LDI, LDR, LEA)
// JMP	1100	    0|00|BR#|000000
// JSR	0100	    1| PC Offset 11
// JSRR	0100	    0|00|BR#|000000
// RET	1100	    0|00|111|000000
// LD	0010	    DR#| PC Offset 9
// LDI	1010	    DR#| PC Offset 9
// LDR	0110	    DR#|BR#|Offset 6
// LEA	1110	    DR#| PC Offset 9
// ST	0011	    SR#| PC Offset 9
// STI	1011	    SR#| PC Offset 9
// STR	0111	    SR#|BR#|Offset 6
// TRAP	1111	    0000 | Trapvec8
// RTI	1000	    000000000000
// Reserved	1101	000000000000

enum class OP {
  BR,
  ADD,
  LD,
  ST,
  JSR,
  AND,
  LDR,
  STR,
  RTI,
  NOT,
  LDI,
  STI,
  JMP,
  Reserved,
  LEA,
  TRAP
};

std::string OP_decode[] = {"BR",  "ADD",      "LD",  "ST",  "JSR", "AND",
                           "LDR", "STR",      "RTI", "NOT", "LDI", "STI",
                           "JMP", "Reserved", "LEA", "TRAP"};

std::string Trap_decode[] = {"GETC", "OUT", "PUTS", "IN", "PUTSP", "HALT"};
;

#endif