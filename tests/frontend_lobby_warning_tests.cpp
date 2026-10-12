#include "halo2/frontend_lobby_warning.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
  try {
    if (argc != 2)
      throw std::runtime_error("original warning fixture argument required");
    std::ifstream file(argv[1]);
    if (!file)
      throw std::runtime_error("missing original warning fixture");
    unsigned count = 0, flags;
    std::uint32_t clock, input, expected;
    while (file >> std::dec >> flags >> clock >> std::hex >> input >> expected) {
      const float alpha = halo2::frontend_lobby_warning_alpha(
          std::bit_cast<float>(input), clock, (flags & 1) != 0);
      if (std::bit_cast<std::uint32_t>(alpha) != expected)
        throw std::runtime_error("warning opacity differs from original22CD48");
      ++count;
    }
    if (!file.eof() || count != 1704)
      throw std::runtime_error("incomplete original warning fixture");
    std::cout << count << " original warning-alpha words matched\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
