#ifndef EMU_INCLUDE
#define EMU_INCLUDE

#include <cstdint>
#include <vector>

typedef uint16_t WORD;

const size_t MEMORY_SIZE = 0x10000;
const size_t WORD_SIZE = 0x10;

void run();
void load(const std::vector<WORD>& prgm);
void reset();

inline void update_input();
inline void update_output();

inline void check_interrupts();
void run_interrupt(uint16_t priority, uint16_t newPC);

inline uint16_t get_bits(uint16_t num, uint16_t idx, uint16_t len);
inline int16_t sign_imm(uint16_t num, uint16_t len);

void set_condition(int16_t num);

inline bool get_halt();
inline void set_halt(bool halt);

void write_OS();

#endif