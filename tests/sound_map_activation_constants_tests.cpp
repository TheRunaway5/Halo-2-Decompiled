#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::uint32_t word(const std::string &raw) {
  check(raw.size() == 8, "Incomplete original map activation operand");
  std::uint32_t result=0;
  for (unsigned i=0;i<4;++i) result|=std::stoul(raw.substr(i*2,2),nullptr,16)<<(i*8);
  return result;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==5,"Supply original map constants fixture and base/1.1/u5 images");
    for (int i=1;i<argc;++i) if (!std::filesystem::exists(argv[i])) return 77;
    check(!halo2::OriginalData::load_xbe(argv[2])->sound_map_activation_constants,
          "Unverified base map activation operands were invented");
    const std::array<std::shared_ptr<const halo2::OriginalData>,2> images{
      halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream fixture(argv[1]);
    unsigned count=0;
    for (std::string line;std::getline(fixture,line);) {
      std::istringstream row(line);
      char kind; unsigned image; std::string one,ambient;
      check(bool(row>>kind>>image>>one>>ambient) && kind=='C' && image==count+1 && image<=images.size(),
            "Incomplete original map activation operand record");
      const auto &actual=images[image-1]->sound_map_activation_constants;
      check(bool(actual),"Verified map activation operands unavailable");
      check(actual->one_bits==word(one) && actual->ambient_transition_bits==word(ambient),
            "Imported original map activation operand bytes differ");
      ++count;
    }
    check(count==2,"Incomplete original map activation image coverage");
    std::cout<<"supportedImages=2 exactImmutableMapOperandBytes=16 baseUnknown=absent "
                 "runtimeEnvironmentSavedOutputAndMapAdmission=separate\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
