#ifndef EMU_INCLUDE
#define EMU_INCLUDE

#include <vector>

typedef unsigned short WORD;

const int MEMORY_SIZE = 0x10000;

void run();
void load(const std::vector<WORD>& prgm);
void load_OS();

void load_debug(const std::vector<WORD>& prgm);
void run_debug();

#endif