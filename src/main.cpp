#include <bitset>
#include <fstream>
#include <iostream>
#include "emu.hpp"

const char help_message[] =
    "Usage:\n"
    "   lc3mu <input.obj>\n";

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "LC3 Emulator\n\n" << help_message;
    return 0;
  }
  if (argc > 2) {
    std::cout << "Too many arguments\n\n" << help_message;
    return 0;
  }

  std::ifstream ifile;
  ifile.open(argv[1]);
  if (!ifile.is_open()) {
    std::cout << "Failed to open file " << argv[1] << "\n\n" << help_message;
    return 0;
  }

  std::vector<char> objsRaw((std::istreambuf_iterator<char>(ifile)),
                            std::istreambuf_iterator<char>());
  std::vector<WORD> objs;
  ifile.close();

  for (size_t i = 0; i < objsRaw.size(); i += 2) {
    objs.push_back((objsRaw[i] << 8) | (objsRaw[i + 1] & 0xFF));
  }

  load_OS();
  load(objs);
  run();

  return 0;
}