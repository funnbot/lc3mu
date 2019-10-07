#include <bitset>
#include <fstream>
#include <iostream>
#include "emu.hpp"

const char help_message[] =
    "Usage:\n"
    "   lc3mu <input.obj>\n"
    "   lc3mu -d <input.obj>\n"
    "Options:\n"
    "   -d --debug      Step through, set breakpoints, and inspect values";

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << "LC3 Emulator\n\n" << help_message;
    return 0;
  }
  std::string in_file = "";
  bool debug = false;
  if (argc == 2) {
    in_file = argv[1];
  } else if (argc == 3) {
    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--debug"))
      debug = true;
    else {
      std::cout << "Invalid option " << argv[1] << std::endl;
      return 0;
    }
    in_file = argv[2];
  }

  std::ifstream ifile;
  ifile.open(in_file);
  if (!ifile.is_open()) {
    std::cout << "Failed to open file " << in_file << "\n\n" << help_message;
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

  if (debug) {
    load_debug(objs);
    run_debug();
  } else {
    load(objs);
    run();
  }

  return 0;
}