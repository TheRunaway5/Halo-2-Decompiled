#include "halo2/material_effects.hpp"
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw halo2::FormatError(
          "Supply map and original sound selection fixtures");
    halo2::Cache cache(argv[1]);
    std::ifstream input(argv[2]);
    unsigned count = 0, foot, action;
    int fallback, material, variant;
    std::array<std::uint32_t, 3> expected;
    while (input >> std::hex >> foot >> std::dec >> fallback >> material >>
           action >> variant >> std::hex >> expected[0] >> expected[1] >>
           expected[2]) {
      const auto actual = halo2::source_material_sound_effects(
          cache, foot, fallback, material, action, variant);
      if (actual != expected)
        throw halo2::FormatError(
            "Sound material selection differs at original case " +
            std::to_string(count));
      ++count;
    }
    if (!input.eof() || count < 1000)
      throw halo2::FormatError("Invalid original sound selection fixture");
    std::cout
        << count
        << " original sound material selections byte exact in both updates\n";
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
