#ifndef EMU_INCLUDE
#define EMU_INCLUDE

#include <cstdint>
#include <vector>

typedef uint16_t WORD;

const int MEMORY_SIZE = 0x10000;

void run();
void load(const std::vector<WORD>& prgm);
void load_OS();
void reset();

inline void update_input();
inline void update_output();

inline WORD get_bits(WORD num, WORD idx, WORD len);
inline int16_t sign_imm(WORD num, WORD len);

void set_condition(int16_t num);
inline bool get_halt();

#endif