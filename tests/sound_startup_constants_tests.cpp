#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t word(const std::string &raw) {
  check(raw.size() == 8, "Incomplete original startup word");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i)
    result |= std::stoul(raw.substr(i*2, 2), nullptr, 16) << (i*8);
  return result;
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 5, "Supply immutable startup fixture and base/1.1/u5 executables");
    for (int i = 1; i < argc; ++i) if (!std::filesystem::exists(argv[i])) return 77;
    check(!halo2::OriginalData::load_xbe(argv[2])->sound_startup_constants,
          "Unverified base startup operands were invented");
    const std::array<std::shared_ptr<const halo2::OriginalData>, 2> images{
      halo2::OriginalData::load_xbe(argv[3]), halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream fixture(argv[1]);
    unsigned rows = 0;
    for (std::string line; std::getline(fixture, line);) {
      std::istringstream row(line);
      char kind; unsigned image; std::string ambient, policy;
      check(bool(row >> kind >> image >> ambient >> policy) && kind == 'C' &&
            image == rows+1 && image <= images.size() && policy.size() == 32,
            "Incomplete paired immutable startup record");
      check(bool(images[image-1]->sound_startup_constants), "Verified startup operands unavailable");
      const auto &actual = *images[image-1]->sound_startup_constants;
      check(actual.ambient_transition_bits == word(ambient), "Original startup scalar differs");
      for (unsigned i = 0; i < actual.voice_policy.size(); ++i)
        check(std::to_integer<unsigned>(actual.voice_policy[i]) == std::stoul(policy.substr(i*2, 2), nullptr, 16),
              "Original startup policy bytes differ");
      ++rows;
    }
    check(rows == 2, "Incomplete supported startup image coverage");
    std::cout << "supportedImages=2 importedImmutableStartupBytes=40 baseUnknown=absent "
                 "mutableStateDeviceAndMapAdmission=separate\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
