#include "emu.hpp"
#include "op.hpp"

#include <iostream>
#include <string>

#define REG(idx) registers[get_bits(instruction, (idx), 3)]
#define DR REG(9)
#define SR1 REG(6)
#define SR2 REG(0)
#define UIMM(len) get_bits(instruction, 0, (len))
#define IMM(len) sign_imm(UIMM(len), len)
#define FLAG(idx) get_bits(instruction, (idx), 1)

// | 0  0  0  0 | 0  0  0  0 | 0  0  0  0 | 0  0  0  0 |
// | 15 14 13 12| 11 10 9  8 | 7  6  5  4 | 3  2  1  0 |

uint16_t memory[MEMORY_SIZE];
uint16_t registers[8];
// Special registers
uint16_t IR = 0, PSR = 0x8002, PC = 0x3000;

//// Memory mapped
// machine control register
uint16_t MCR = 0xFFFE;
// IO
uint16_t KBSR = 0xFE00, KBDR = 0xFE02, DSR = 0xFE04, DDR = 0xFE06;

void run() {
  for (;;) {
    update_input();

    uint16_t instruction = memory[PC++];
    OP opcode = static_cast<OP>(get_bits(instruction, 12, 4));

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
        if ((get_bits(PSR, 0, 3) & get_bits(instruction, 9, 3))) PC += IMM(9);
        break;

      case OP::JMP:
        PC = static_cast<int16_t>(SR1) - 1;
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

    if (get_halt()) return;
    update_output();
  }
}

void load(const std::vector<WORD>& prgm) {
  uint16_t orig = prgm[0];
  for (size_t i = 1; i < prgm.size(); i++) memory[(i - 1) + orig] = prgm[i];
  // fill in starting at origin, i-1 because we skip the orig instruction
  PC = orig;
}

void reset() {
  for (int i = 0; i < 8; i++) registers[i] = 0;
  PC = 0x3000;
  IR = 0;
  PSR = 0x8002;
}

inline void update_input() {
  if ((memory[KBSR] & 0x8000) == 0) {  // keyboard ready
    std::cin >> memory[KBDR];
    memory[KBDR] &= 0x00FF;  // Chop upper 8 bits
    memory[KBSR] |= 0x8000;  // reset ready bit
  }
}

inline void update_output() {
  if (memory[DDR] != 0) {                                  // display ready
    std::cout << static_cast<char>(memory[DDR] & 0x00FF);  // Chop upper 8 bits
    memory[DDR] = 0;
  }
}

// Get bits from idx to idx + len of a uint16
inline uint16_t get_bits(uint16_t num, uint16_t idx, uint16_t len) {
  return ((num >> idx) & ~(0xFFFF << len));
}

// val < 2^len
// inline uint16_t set_bits(uint16_t num, uint16_t idx, uint16_t len,
//                          uint16_t val) {
//   return (num & ~(~(0xFFFF << len) << idx)) | (val << idx);
// }

// convert num of arbitrary bit len to 16 bit signed
inline int16_t sign_imm(uint16_t num, uint16_t len) {
  return get_bits(num, len - 1, 1) ? (~((num ^ ~((~0U) << len)) + 1) + 1) : num;
}

// psr bits 0 - 2
void set_condition(int16_t num) {
  PSR &= 0xFFF8;
  PSR |= ((num > 0 ? 1U : 0) | (num == 0 ? 2U : 0) | (num < 0 ? 4U : 0));
}

// mcr bit 15 (1 clock is running)
inline bool get_halt() { return (memory[MCR] & 0x8000) == 0; }
inline void set_halt(bool halt) {
  halt ? memory[MCR] &= 0x7FFF : memory[MCR] |= 0x8000;
}

void write_OS() {
  // Trap vector table (valid entries)
  memory[0x0020] = 0x0400;
  memory[0x0021] = 0x0430;
  memory[0x0022] = 0x0450;
  memory[0x0023] = 0x04A0;
  memory[0x0024] = 0x04E0;
  memory[0x0025] = 0xFD70;
  // Implementation of GETC
  memory[0x0400] = 0x3E07;
  memory[0x0401] = 0x5020;
  memory[0x0402] = 0xB003;
  memory[0x0403] = 0xA003;
  memory[0x0404] = 0x2E03;
  memory[0x0405] = 0xC1C0;
  memory[0x0406] = 0xFE00;
  memory[0x0407] = 0xFE02;
  // Implementation of OUT
  memory[0x0430] = 0x3E0A;
  memory[0x0431] = 0x3208;
  memory[0x0432] = 0xA205;
  memory[0x0433] = 0x07FE;
  memory[0x0434] = 0xB004;
  memory[0x0435] = 0x2204;
  memory[0x0436] = 0x2E04;
  memory[0x0437] = 0xC1C0;
  memory[0x0438] = 0xFE04;
  memory[0x0439] = 0xFE06;
  // Implementation of PUTS
  memory[0x0450] = 0x3E16;
  memory[0x0451] = 0x3012;
  memory[0x0452] = 0x3212;
  memory[0x0453] = 0x3412;
  memory[0x0454] = 0x6200;
  memory[0x0455] = 0x0405;
  memory[0x0456] = 0xA409;
  memory[0x0457] = 0x07FE;
  memory[0x0458] = 0xB208;
  memory[0x0459] = 0x1021;
  memory[0x045A] = 0x0FF9;
  memory[0x045B] = 0x2008;
  memory[0x045C] = 0x2208;
  memory[0x045D] = 0x2408;
  memory[0x045E] = 0x2E08;
  memory[0x045F] = 0xC1C0;
  memory[0x0460] = 0xFE04;
  memory[0x0461] = 0xFE06;
  memory[0x0462] = 0xF3FD;
  memory[0x0463] = 0xF3FE;
  // Implementation of IN
  memory[0x04A0] = 0x3E06;  // ST R7, SaveR7
  memory[0x04A1] = 0xE006;  // LEA R0, Message
  memory[0x04A2] = 0xF022;  // PUTS
  memory[0x04A3] = 0xF020;  // GETC
  memory[0x04A4] = 0xF021;  // OUT
  memory[0x04A5] = 0x2E01;  // LD R7, SaveR7
  memory[0x04A6] = 0xC1C0;  // RET
  memory[0x04A7] = 0x3001;  // SaveR7 (.BLKW #1)
  /* the "Input a character> " message goes here */
  std::string inputPrompt = "Input a character> \0";
  for (size_t i = 0; i < inputPrompt.size(); i++) {
    memory[0x04A8 + i] = inputPrompt[i];
  }

  // Implementation of PUTSP
  memory[0x04E0] = 0x3E27;
  memory[0x04E1] = 0x3022;
  memory[0x04E2] = 0x3222;
  memory[0x04E3] = 0x3422;
  memory[0x04E4] = 0x3622;
  memory[0x04E5] = 0x1220;
  memory[0x04E6] = 0x6040;
  memory[0x04E7] = 0x0406;
  memory[0x04E8] = 0x480D;
  memory[0x04E9] = 0x2418;
  memory[0x04EA] = 0x5002;
  memory[0x04EB] = 0x0402;
  memory[0x04EC] = 0x1261;
  memory[0x04ED] = 0x0FF8;
  memory[0x04EE] = 0x2014;
  memory[0x04EF] = 0x4806;
  memory[0x04F0] = 0x2013;
  memory[0x04F1] = 0x2213;
  memory[0x04F2] = 0x2413;
  memory[0x04F3] = 0x2613;
  memory[0x04F4] = 0x2E13;
  memory[0x04F5] = 0xC1C0;
  memory[0x04F6] = 0x3E06;
  memory[0x04F7] = 0xA607;
  memory[0x04F8] = 0x0801;
  memory[0x04F9] = 0x0FFC;
  memory[0x04FA] = 0xB003;
  memory[0x04FB] = 0x2E01;
  memory[0x04FC] = 0xC1C0;
  memory[0x04FE] = 0xFE06;
  memory[0x04FF] = 0xFE04;
  memory[0x0500] = 0xF3FD;
  memory[0x0501] = 0xF3FE;
  memory[0x0502] = 0xFF00;
  // Implementation of HALT
  memory[0xFD00] = 0x3E3E;
  memory[0xFD01] = 0x303C;
  memory[0xFD02] = 0x2007;
  memory[0xFD03] = 0xF021;
  memory[0xFD04] = 0xE006;
  memory[0xFD05] = 0xF022;
  memory[0xFD06] = 0xF025;
  memory[0xFD07] = 0x2036;
  memory[0xFD08] = 0x2E36;
  memory[0xFD09] = 0xC1C0;
  memory[0xFD70] = 0x3E0E;
  memory[0xFD71] = 0x320C;
  memory[0xFD72] = 0x300A;
  memory[0xFD73] = 0xE00C;
  memory[0xFD74] = 0xF022;
  memory[0xFD75] = 0xA22F;
  memory[0xFD76] = 0x202F;
  memory[0xFD77] = 0x5040;
  memory[0xFD78] = 0xB02C;
  memory[0xFD79] = 0x2003;
  memory[0xFD7A] = 0x2203;
  memory[0xFD7B] = 0x2E03;
  memory[0xFD7C] = 0xC1C0;
  /* the "halting the processor" message goes here */
  std::string haltMessage = "\n----- Halting the processor ----- \n\0";
  for (size_t i = 0; i < haltMessage.size(); i++) {
    memory[0xFD80 + i] = haltMessage[i];
  }

  memory[0xFDA5] = 0xFFFE;
  memory[0xFDA6] = 0x7FFF;
  // Keyboard status register
  memory[0xFE00] = 0x8000;
  // Display status register
  memory[0xFE04] = 0x8000;
  // Machine control register
  memory[0xFFFE] = 0xFFFF;

  // Bad traps
  for (int i = 0; i < 0xFF; i++) {
    if (memory[i] == 0) memory[i] = 0xFD00;
  }
}
