#include "emu.hpp"
#include "op.hpp"
#include "os.hpp"

#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <sstream>

#define REG(ins, idx) registers[GET_BITS((ins), (idx), 3)]
#define UIMM(ins, len) GET_BITS((ins), 0, (len))
#define IMM(ins, len) sign_imm(UIMM((ins), (len)), (len))
#define TEST_FLAG(ins, idx) (ins & (1 << idx))

#define GET_BITS(num, idx, len) ((num >> idx) & ~(0xFFFF << len))
#define IS_RUNNING() (*MCR & 0x8000)

namespace {

const WORD DR = 9, SR1 = 6, SR2 = 0;

WORD memory[MEMORY_SIZE];
WORD registers[8];
// Special registers
WORD IR = 0, PSR = 0x8002, PC = 0x3000;

//// Memory mapped
// machine control register
WORD *MCR = memory + 0xFFFE;
// IO
WORD *KBSR = memory + 0xFE00, *KBDR = memory + 0xFE02, *DSR = memory + 0xFE04,
     *DDR = memory + 0xFE06;

inline void update_io() {
  if ((*KBSR & 0x8000) != 0) {  // keyboard ready
    putchar(10);
    char *c = getpass("");
    *KBDR = *c;
    *KBSR = 0;  // reset ready bit
  }

  if (*DDR != 0) {  // display ready
    putchar(*DDR & 0x00FF);
    *DDR = 0;
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

void reset() {
  for (int i = 0; i < 8; i++) registers[i] = 0;
  PC = 0x3000;
  IR = 0;
  PSR = 0x8002;
}

}  // namespace

void run() {
  while (IS_RUNNING()) {
    IR = memory[PC++];
    OP opcode = static_cast<OP>(GET_BITS(IR, 12, 4));
    // std::cout << OP_decode[GET_BITS(IR, 12, 4)] << " \n";
    switch (opcode) {
      case OP::ADD:
        set_condition(REG(IR, DR) =
                          REG(IR, SR1) +
                          (TEST_FLAG(IR, 5) ? IMM(IR, 5) : REG(IR, SR2)));
        break;
      case OP::AND:
        set_condition(REG(IR, DR) =
                          REG(IR, SR1) &
                          (TEST_FLAG(IR, 5) ? IMM(IR, 5) : REG(IR, SR2)));
        break;
      case OP::NOT:
        set_condition(REG(IR, DR) = ~REG(IR, SR1));
        break;

      case OP::BR:
        if ((GET_BITS(PSR, 0, 3) & GET_BITS(IR, 9, 3))) PC += IMM(IR, 9);
        break;

      case OP::JMP:
        PC = REG(IR, SR1) - 1;
        break;
      case OP::JSR:
        registers[7] = PC;
        PC += (TEST_FLAG(IR, 11) ? IMM(IR, 11) : REG(IR, SR1));
        break;

      case OP::LD:
        set_condition(REG(IR, DR) = memory[PC + IMM(IR, 9)]);
        break;
      case OP::LDI:
        set_condition(REG(IR, DR) = memory[memory[PC + IMM(IR, 9)]]);
        break;
      case OP::LDR:
        set_condition(REG(IR, DR) = memory[REG(IR, SR1) + IMM(IR, 6)]);
        break;
      case OP::LEA:
        set_condition(REG(IR, DR) = PC + IMM(IR, 9));
        break;

      case OP::ST:
        memory[PC + IMM(IR, 9)] = REG(IR, DR);
        break;
      case OP::STI:
        memory[memory[PC + IMM(IR, 9)]] = REG(IR, DR);
        break;
      case OP::STR:
        memory[REG(IR, SR1) + IMM(IR, 6)] = REG(IR, DR);
        break;

      case OP::TRAP:
        registers[7] = PC + 1;
        PC = memory[UIMM(IR, 8)];
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

void load(const std::vector<WORD> &prgm) {
  WORD orig = prgm[0];
  for (size_t i = 1; i < prgm.size(); i++) memory[(i - 1) + orig] = prgm[i];
  // fill in starting at origin, i-1 because we skip the orig instruction
  PC = orig;
}

void load_OS() { write_OS(memory); }

namespace {

WORD LAST_PC = 0x3000;
size_t PRGM_ORIG = 0, PRGM_SIZE = 0;
std::vector<WORD> B_POINTS;
int SUB_LEVEL = 0;

// clang-format off
char help_message[] =
    "Commands:\n"
    "   h,   help                        Print this message.\n"
    "\n"
    "   pr,  print                       Print instructions just loaded.\n"
    "   s,   step                        Execute instruction.\n"
    "   n,   next                        Fully execute instruction or "
    "subroutine.\n"
    "   f,   finish                      Finish the current subroutine.\n"
    "   r,   run                         Run indefinitely.\n"
    "   uh,  unhalt                      Continue running after halt.\n"
    "   q,   quit                        Quit the debugging session.\n"
    "\n"
    "   m,   memory <hex>                Inspect memory address <hex>.\n"
    "   mr,  memoryrange <hex> <hex>     Inspect memory address range <hex> to "
    "<hex>.\n"
    "   sm,  setmemory <hex> <uint>      Set address <hex> in memory to "
    "<uint>.\n"
    "\n"
    "   r,   registers                   Inspect register values (R0-R7, PC, "
    "PSR, IR).\n"
    "   sr,  setregister <reg> <uint>    Set register <reg> (R0-R7, PC, PSR, "
    "IR) to <uint> value.\n"
    "   cr,  clearregisters              Clear registers R0-R7.\n"
    "   rr,  resetregisters              Reset all registers (R0-R7, PC, PSR, "
    "IR).\n"
    "   pc,  programcounter <hex>        Set PC to address <hex>.\n"
    "\n"
    "   bp,  breakpoint <hex>            Set or clear a breakpoint when PC "
    "equals <hex> value.\n"
    "   cbp, clearbreakpoints            Clear all set breakpoints.\n"
    "\n"
    "   pb,  printoutput [format=char]   Print output buffer as [format=char] "
    "(char, int, uint, hex).\n"
    "   om,  outputmode [format=char]    Print OUT as [format=char] (char, "
    "int, uint, hex).\n";
// clang-format on

inline std::string get_line() {
  std::string input;
  std::getline(std::cin, input);
  return input;
}

bool try_parse_int(const std::string &str, int *out, int base = 10) {
  try {
    *out = std::stoi(str, nullptr, base);
    return true;
  } catch (std::invalid_argument e) {
    *out = 0;
    return false;
  }
}

void print_condition(WORD ins) {
  if (TEST_FLAG(ins, 11)) std::cout << 'n';
  if (TEST_FLAG(ins, 10)) std::cout << 'z';
  if (TEST_FLAG(ins, 9)) std::cout << 'p';
}

#define SPLIT_STR(str, vec_str) \
  std::istringstream iss(str);  \
  for (std::string s; iss >> s;) vec_str.push_back(s)

void print_instruction(WORD idx) {
  WORD ins = memory[idx];
  int opcode = GET_BITS(ins, 12, 4);
  OP op = static_cast<OP>(opcode);
  auto flags(std::cout.flags());
  std::cout << std::setfill('0') << std::setw(4) << std::hex << std::uppercase
            << idx << ": x" << std::setw(4) << ins << " ";
  auto hex_flags(std::cout.flags());
  std::cout.flags(flags);

#define P_OP() (OP_decode[opcode])
#define P_REG(idx) 'R' << GET_BITS(ins, idx, 3)
#define P_IMM(len) '#' << IMM(ins, len)
#define P_OFFSET(len)                                       \
  std::cout.flags(hex_flags);                               \
  std::cout << "x" << std::setw(4)                          \
            << (static_cast<int>(idx) + IMM(ins, len) + 1); \
  std::cout.flags(flags);
  // clang-format off
  switch (op) {
    case OP::LD: case OP::LDI: case OP::ST: case OP::STI: case OP::LEA: {
      std::cout << P_OP() << ' ' << P_REG(DR) << ", ";
      P_OFFSET(9);
      break;
    }
    case OP::ADD: case OP::AND: {
      std::cout << P_OP() << ' ' << P_REG(DR) << ", " << P_REG(SR1) << ", ";
      if (TEST_FLAG(ins, 5))
        std::cout << P_IMM(5);
      else std::cout << P_REG(SR2);
      break;
    }
    case OP::LDR: case OP::STR: {
      std::cout << P_OP() << ' ' << P_REG(DR) << ", " 
                << P_REG(SR1) << ", " << P_IMM(6);
      break;
    }
    case OP::JMP: {
      std::cout << P_OP() << " ";
      P_OFFSET(11);
      break;
    }
    case OP::BR: {
      if (GET_BITS(ins, DR, 3) != 0) { 
        std::cout << P_OP();
        print_condition(ins);
        std::cout << " ";
        P_OFFSET(9);
      } else {
        std::cout << "NOP";
        if (isprint(ins)) std::cout << " '" << static_cast<char>(ins) << "'";
      }
      break;
    }
    case OP::NOT: {
      std::cout << P_OP() << ' ' << P_REG(DR) << ", " << P_REG(SR1);
      break;
    }
    case OP::JSR: {
      std::cout << P_OP();
      break;
    }
    case OP::TRAP: {
      int trapvec = UIMM(ins, 8) - 32;
      if (trapvec > 6 || trapvec < 0) 
        std::cout << "BADTRAP ";
      else std::cout << Trap_decode[trapvec] << " ";
      break;
    }
    case OP::RTI:
    case OP::Reserved:
      std::cout << P_OP();
      break;
    default:
      std::cout << static_cast<int>(ins);
      break;
  }
// clang-format on
#undef P_OP
#undef P_REG
#undef P_OFFSET
#undef P_IMM
}

void step_instruction() {
  LAST_PC = PC;
  IR = memory[PC++];
  OP opcode = static_cast<OP>(GET_BITS(IR, 12, 4));
  switch (opcode) {
    case OP::ADD:
      set_condition(REG(IR, DR) =
                        REG(IR, SR1) +
                        (TEST_FLAG(IR, 5) ? IMM(IR, 5) : REG(IR, SR2)));
      break;
    case OP::AND:
      set_condition(REG(IR, DR) =
                        REG(IR, SR1) &
                        (TEST_FLAG(IR, 5) ? IMM(IR, 5) : REG(IR, SR2)));
      break;
    case OP::NOT:
      set_condition(REG(IR, DR) = ~REG(IR, SR1));
      break;

    case OP::BR:
      if ((GET_BITS(PSR, 0, 3) & GET_BITS(IR, 9, 3))) PC += IMM(IR, 9);
      break;

    case OP::JMP:
      PC = REG(IR, SR1) - 1;
      SUB_LEVEL += (GET_BITS(IR, SR1, 3) == 7) ? -1 : 1;
      break;
    case OP::JSR:
      registers[7] = PC;
      PC += (TEST_FLAG(IR, 11) ? IMM(IR, 11) : REG(IR, SR1));
      SUB_LEVEL++;
      break;

    case OP::LD:
      set_condition(REG(IR, DR) = memory[PC + IMM(IR, 9)]);
      break;
    case OP::LDI:
      set_condition(REG(IR, DR) = memory[memory[PC + IMM(IR, 9)]]);
      break;
    case OP::LDR:
      set_condition(REG(IR, DR) = memory[REG(IR, SR1) + IMM(IR, 6)]);
      break;
    case OP::LEA:
      set_condition(REG(IR, DR) = PC + IMM(IR, 9));
      break;

    case OP::ST:
      memory[PC + IMM(IR, 9)] = REG(IR, DR);
      break;
    case OP::STI:
      memory[memory[PC + IMM(IR, 9)]] = REG(IR, DR);
      break;
    case OP::STR:
      memory[REG(IR, SR1) + IMM(IR, 6)] = REG(IR, DR);
      break;

    case OP::TRAP:
      registers[7] = PC + 1;
      PC = memory[UIMM(IR, 8)];
      SUB_LEVEL++;
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

bool is_breakpoint(WORD pc) {
  return std::find(B_POINTS.begin(), B_POINTS.end(), pc) != B_POINTS.end();
}

void print_current() {
  std::cout << "> ";
  print_instruction(LAST_PC);
  std::cout << std::endl;
  std::cout << "  ";
  print_instruction(PC);
  std::cout << std::endl;
}

}  // namespace

void run_debug() {
  for (;;) {
    std::cout << ">> ";
    std::string input = get_line();
    if (input == "") continue;
    std::vector<std::string> cmds;
    SPLIT_STR(input, cmds);

#define CMD(s1, s2) (cmds[0] == s1 || cmds[0] == s2)
    if (CMD("h", "help")) {
      std::cout << help_message;
    } else if (CMD("pr", "print")) {
      int start = PRGM_ORIG, end = PRGM_ORIG + PRGM_SIZE;
      for (int i = start; i < end; i++) {
        print_instruction(i);
        std::cout << std::endl;
      }
    } else if (CMD("s", "step")) {
      if (IS_RUNNING()) {
        step_instruction();
        print_current();
      }
    } else if (CMD("n", "next")) {
      int c_level = SUB_LEVEL;
      if (IS_RUNNING()) {
        step_instruction();
        print_current();
      }
      if (c_level < SUB_LEVEL) {
        while (IS_RUNNING()) {
          step_instruction();
          if (is_breakpoint(PC) || c_level == SUB_LEVEL) {
            std::cout << "\n";
            print_current();
            break;
          }
        }
      }
    } else if (CMD("r", "run")) {
      while (IS_RUNNING()) {
        step_instruction();
        if (is_breakpoint(PC)) {
          print_current();
          break;
        }
      }
    } else if (CMD("q", "quit")) {
      std::cout << "Terminating session.\n";
      return;
    } else {
      std::cout << "Invalid command, try 'help'.\n";
    }
#undef CMD
  }
}

void load_debug(const std::vector<WORD> &prgm) {
  PRGM_ORIG = prgm[0];
  PRGM_SIZE = prgm.size() - 1;
  for (size_t i = 1; i <= PRGM_SIZE; i++) memory[(i - 1) + PRGM_ORIG] = prgm[i];
  // fill in starting at origin, i-1 because we skip the orig instruction
  PC = PRGM_ORIG;
}

#undef SPLIT_STR