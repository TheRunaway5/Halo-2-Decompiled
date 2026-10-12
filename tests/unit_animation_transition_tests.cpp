#include "halo2/source_runtime_image.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/unit_animation_runtime.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
int main(int argc, char **argv) {
  try {
    if (argc != 3)
      throw std::runtime_error("Supply original transition fixture and image");
    if (!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))
      return 77;
    const auto memory = halo2::load_source_runtime_image_1_1(argv[2]);
    std::ifstream input(argv[1]);
    std::size_t cases = 0;
    for (std::string line; std::getline(input, line);) {
      std::istringstream in(line);
      char record;
      std::uint32_t old, activity, changed, flag, minimum, cw, mx, expected;
      if (!(in >> record >> std::hex >> old >> activity >> changed >> flag >>
            minimum >> cw >> mx >> expected) ||
          record != 'C')
        throw std::runtime_error("Original transition record");
      const auto actual = halo2::source_unit_animation_transition_1_1(
          memory, old, activity, changed, flag, minimum,
          {std::uint16_t(cw), mx});
      if (actual != expected) {
        std::ostringstream out;
        out << "Transition " << cases << " actual " << std::hex << actual
            << " expected " << expected;
        throw std::runtime_error(out.str());
      }
      ++cases;
    }
    if (cases != 20736)
      throw std::runtime_error("Incomplete original transition matrix");
    std::cout << cases
              << " original animation transition cases, four FP modes match\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
