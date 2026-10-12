#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t word(const std::string &raw) {
  check(raw.size() == 8, "Incomplete original sound cache callback operand");
  std::uint32_t result = 0;
  for (unsigned i = 0; i < 4; ++i) result |= std::stoul(raw.substr(i*2, 2), nullptr, 16) << (i*8);
  return result;
}
}
int main(int argc, char **argv) {
  try {
    check(argc == 5, "Supply sound cache callback fixture and base/1.1/u5 images");
    for (int i = 1; i < argc; ++i) if (!std::filesystem::exists(argv[i])) return 77;
    check(!halo2::OriginalData::load_xbe(argv[2])->sound_cache_startup_constants,
          "Unverified base callback identities were invented");
    const std::array<std::shared_ptr<const halo2::OriginalData>, 2> images{
      halo2::OriginalData::load_xbe(argv[3]), halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream fixture(argv[1]); unsigned rows = 0;
    for (std::string line; std::getline(fixture, line);) {
      std::istringstream row(line); char kind; unsigned image; std::string deleted, busy;
      check(bool(row >> kind >> image >> deleted >> busy) && kind == 'C' && image == rows+1 && image <= 2,
            "Incomplete paired sound cache callback record");
      check(bool(images[image-1]->sound_cache_startup_constants), "Verified cache callback identities unavailable");
      const auto &actual = *images[image-1]->sound_cache_startup_constants;
      check(actual.delete_callback_identity == word(deleted) && actual.busy_callback_identity == word(busy),
            "Actual imported sound cache callback immediates differ");
      ++rows;
    }
    check(rows == 2, "Incomplete sound cache callback image coverage");
    std::cout << "supportedImages=2 importedCallbackOperandBytes=16 baseUnknown=absent "
                 "nativeCallbacksCacheLifetimeAndAdmission=separate\n";
  } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
