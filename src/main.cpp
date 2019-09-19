#include <bitset>
#include <fstream>
#include <iostream>

#include "emu.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "Provide a input file.\n";
    return 0;
  }
  if (argc > 2) {
    std::cout << "Too many arguments.\n";
    return 0;
  }

  std::ifstream ifile;
  ifile.open(argv[1]);
  if (!ifile.is_open()) {
    std::cout << "Invalid file.\n";
    return 0;
  }

  std::vector<char> objsRaw((std::istreambuf_iterator<char>(ifile)),
                            std::istreambuf_iterator<char>());
  std::vector<WORD> objs;

  for (size_t i = 0; i < objsRaw.size(); i += 2) {
    objs.push_back((objsRaw[i] << 8) | (objsRaw[i + 1] & 0xFF));
  }

  load_OS();
  load(objs);
  run();

  return 0;
}