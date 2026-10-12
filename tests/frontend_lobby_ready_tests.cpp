#include "halo2/frontend_lobby_ready.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace halo2;
int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::runtime_error("original ready fixture argument required");
    std::ifstream file(argv[1]);
    if (!file) throw std::runtime_error("missing original ready fixture");
    unsigned count = 0;
    FrontendLobbyReadyInput input; int expected;
    while (file >> input.session_enum >> input.present_count >> input.ready_count >> input.team_mode
                >> input.distinct_team_count >> input.maximum_team_count >> expected) {
      if (frontend_lobby_ready_status(input) != expected)
        throw std::runtime_error("local ready status differs from original19A2CE");
      ++count;
    }
    if (!file.eof() || count != 1014) throw std::runtime_error("incomplete original ready fixture");
    std::cout << count << " original ready-status cases matched\n";
    return 0;
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
