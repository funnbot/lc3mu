#include <unistd.h>
#include <fstream>
#include <iostream>
#include <vector>
#include "os.hpp"

typedef unsigned short WORD;
typedef short SWORD;

// clang-format off
enum class OP {
    BR, ADD, LD, ST, JSR, AND, LDR, STR, RTI, NOT, LDI, STI, JMP, Reserved, LEA, TRAP
};
// clang-format on

void run();
void load(const std::vector<WORD> &prgm);
void load_OS();

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cout << "LC3 Emulator\nUsage:\n  lc3mu <input.obj>\n";
        return 0;
    }
    std::string in_file = argv[1];

    std::ifstream ifile(in_file);
    if (!ifile.is_open()) {
        std::cout << "Failed to open file \"" << in_file << "\".\n";
        return 0;
    }

    std::vector<char> bytes((std::istreambuf_iterator<char>(ifile)),
                            (std::istreambuf_iterator<char>()));
    std::vector<WORD> objs;
    objs.reserve(bytes.size() / 2);

    for (size_t i = 0; i < bytes.size() - 1; i += 2)
        objs.push_back(bytes[i] << 8 | (bytes[i + 1] & 0xFF));

    load_OS();
    load(objs);
    std::cout << "Executing...\n\n";
    run();

    return 0;
}

const int MEMORY_SIZE = 0x10000;

WORD memory[MEMORY_SIZE] = {0};
WORD registers[8] = {0};
// Special registers
WORD IR = 0, PSR = 0x8002, PC = 0x3000;

//// Memory mapped
// machine control register
WORD *MCR = memory + 0xFFFE;
// IO
WORD *KBSR = memory + 0xFE00, *KBDR = memory + 0xFE02, *DSR = memory + 0xFE04,
     *DDR = memory + 0xFE06;

// convert num of arbitrary bit len to 16 bit signed
SWORD sign_imm(WORD num, WORD len) {
    if (num & 1 << (len - 1)) return -((num ^ ~(0xFFFF << len)) + 1);
    return num;
}

// psr bits 0 - 2
void set_condition(SWORD num) {
    PSR &= 0xFFF8;
    if (num > 0)
        PSR |= 1U;
    else if (num == 0)
        PSR |= 2U;
    else if (num < 0)
        PSR |= 4U;
}

#define GET_BITS(num, idx, len) ((num >> idx) & ~(0xFFFF << len))
#define REG(idx) registers[GET_BITS(IR, (idx), 3)]
#define UIMM(len) GET_BITS(IR, 0, (len))
#define IMM(len) sign_imm(UIMM((len)), (len))
#define TEST_FLAG(idx) (IR & (1 << idx))

#define DR REG(9)
#define SR1 REG(6)
#define SR2 REG(0)
void run() {
    while (*MCR & 0x8000) {
        IR = memory[PC++];
        switch (static_cast<OP>(GET_BITS(IR, 12, 4))) {
            case OP::ADD:
                set_condition(DR = SR1 + (TEST_FLAG(5) ? IMM(5) : SR2));
                break;
            case OP::AND:
                set_condition(DR = SR1 & (TEST_FLAG(5) ? IMM(5) : SR2));
                break;
            case OP::NOT: set_condition(DR = ~SR1); break;

            case OP::BR:
                if ((GET_BITS(PSR, 0, 3) & GET_BITS(IR, 9, 3))) PC += IMM(9);
                break;

            case OP::JMP: PC = SR1 - 1; break;
            case OP::JSR:
                registers[7] = PC;
                PC += (TEST_FLAG(11) ? IMM(11) : SR1);
                break;

            case OP::LD: set_condition(DR = memory[PC + IMM(9)]); break;
            case OP::LDI:
                set_condition(DR = memory[memory[PC + IMM(9)]]);
                break;
            case OP::LDR: set_condition(DR = memory[SR1 + IMM(6)]); break;
            case OP::LEA: set_condition(DR = PC + IMM(9)); break;

            case OP::ST: memory[PC + IMM(9)] = DR; break;
            case OP::STI: memory[memory[PC + IMM(9)]] = DR; break;
            case OP::STR: memory[SR1 + IMM(6)] = DR; break;

            case OP::TRAP:
                registers[7] = PC + 1;
                PC = memory[UIMM(8)];
                break;

            case OP::RTI:
            case OP::Reserved: return;
        }

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
}
#undef GET_BITS
#undef REG
#undef UIMM
#undef IMM
#undef TEST_FLAG
#undef DR
#undef SR1
#undef SR2

void load(const std::vector<WORD> &prgm) {
    WORD orig = prgm[0];
    for (size_t i = 1; i < prgm.size(); i++) memory[(i - 1) + orig] = prgm[i];
    // fill in starting at origin, i-1 because we skip the orig instruction
    PC = orig;
}

void load_OS() { write_OS(memory); }