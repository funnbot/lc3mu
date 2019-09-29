#include "emu.hpp"
#include "op.hpp"
#include "os.hpp"

#include <iostream>
#include <string>
  
#define REG(idx) registers[GET_BITS(IR, (idx), 3)]
#define DR REG(9)
#define SR1 REG(6)
#define SR2 REG(0)
#define UIMM(len) GET_BITS(IR, 0, (len))
#define IMM(len) sign_imm(UIMM(len), len)
#define FLAG(idx) (IR & (1 << idx))

#define GET_BITS(num, idx, len) ((num >> idx) & ~(0xFFFF << len))
#define IS_RUNNING() (memory[MCR] & 0x8000)

WORD memory[MEMORY_SIZE];
WORD registers[8];
// Special registers
WORD IR = 0, PSR = 0x8002, PC = 0x3000;

//// Memory mapped
// machine control register
WORD MCR = 0xFFFE;
// IO
WORD KBSR = 0xFE00, KBDR = 0xFE02, DSR = 0xFE04, DDR = 0xFE06;

inline void update_io() {
  if ((memory[KBSR] >> 15) == 1) {  // keyboard ready
    char c;
    std::cin >> c;
    memory[KBDR] = c;
    memory[KBSR] = 0;  // reset ready bit
  }

  if (memory[DDR] != 0) {                                  // display ready
    std::cout << static_cast<char>(memory[DDR] & 0x00FF);  // Chop upper 8 bits
    memory[DDR] = 0;
  }
}

// convert num of arbitrary bit len to 16 bit signed
inline int16_t sign_imm(WORD num, WORD len) {
  if (num & 1 << (len - 1)) return -((num ^ ~(0xFFFF << len)) + 1);
  return num;
}

// psr bits 0 - 2
void set_condition(int16_t num) {
  PSR &= 0xFFF8;
  if (num > 0) PSR |= 1U;
  if (num == 0) PSR |= 2U;
  if (num < 0) PSR |= 4U;
}

void run() {
  while (IS_RUNNING()) {
    IR = memory[PC++];
    OP opcode = static_cast<OP>(GET_BITS(IR, 12, 4));

    switch (opcode) {
      case OP::ADD:
        set_condition(DR = FLAG(5) ? SR1 + IMM(5) : SR1 + SR2);
        break;
      case OP::AND:
        set_condition(DR = FLAG(5) ? SR1 & IMM(5) : SR1 & SR2);
        break;
      case OP::NOT:
        set_condition(DR = ~SR1);
        break;

      case OP::BR:
        if ((GET_BITS(PSR, 0, 3) & GET_BITS(IR, 9, 3))) PC += IMM(9);
        break;

      case OP::JMP:
        PC = SR1 - 1;
        break;
      case OP::JSR:
        registers[7] = PC;
        PC += (FLAG(11) ? IMM(11) : SR1);
        break;

      case OP::LD:
        set_condition(DR = memory[PC + IMM(9)]);
        break;
      case OP::LDI:
        set_condition(DR = memory[memory[PC + IMM(9)]]);
        break;
      case OP::LDR:
        set_condition(DR = memory[SR1 + IMM(6)]);
        break;
      case OP::LEA:
        set_condition(DR = PC + IMM(9));
        break;

      case OP::ST:
        memory[PC + IMM(9)] = DR;
        break;
      case OP::STI:
        memory[memory[PC + IMM(9)]] = DR;
        break;
      case OP::STR:
        memory[SR1 + IMM(6)] = DR;
        break;

      case OP::TRAP:
        registers[7] = PC + 1;
        PC = memory[UIMM(8)];
        break;

      case OP::RTI:
        // unused
        std::cerr << "Privilege mode exception" << std::endl;
        return;

      case OP::Reserved:
        std::cerr << "Illegal opcode exception" << std::endl;
        return;
    }

    update_io();
  }
}

void load(const std::vector<WORD>& prgm) {
  WORD orig = prgm[0];
  for (size_t i = 1; i < prgm.size(); i++) memory[(i - 1) + orig] = prgm[i];
  // fill in starting at origin, i-1 because we skip the orig instruction
  PC = orig;
}

void load_OS() { write_OS(memory); }

void reset() {
  for (int i = 0; i < 8; i++) registers[i] = 0;
  PC = 0x3000;
  IR = 0;
  PSR = 0x8002;
}
