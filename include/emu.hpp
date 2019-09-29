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

#endif