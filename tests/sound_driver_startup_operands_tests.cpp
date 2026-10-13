#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool condition,const char *message) {
  if (!condition) throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &raw) {
  check(!raw.empty() && raw.size()%2==0,"Incomplete original driver operand");
  std::vector<std::byte> result(raw.size()/2);
  for (std::size_t i=0;i<result.size();++i)
    result[i]=std::byte(std::stoul(raw.substr(i*2,2),nullptr,16));
  return result;
}
std::uint32_t word(const std::string &raw) {
  const auto b=bytes(raw);check(b.size()==4,"Incomplete original driver word");
  std::uint32_t result=0;
  for (unsigned i=0;i<4;++i) result|=std::to_integer<std::uint32_t>(b[i])<<(8*i);
  return result;
}
}
int main(int argc,char **argv) {
  try {
    check(argc==5,"Supply original driver operands and base/1.1/u5 images");
    for (int i=1;i<argc;++i) if (!std::filesystem::exists(argv[i])) return 77;
    const auto base=halo2::OriginalData::load_xbe(argv[2]);
    check(bool(base->sound_driver_startup_operands),"Verified original disc basis operands unavailable");
    const std::array<std::shared_ptr<const halo2::OriginalData>,2> images{
        halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    std::ifstream input(argv[1]);unsigned count=0;
    for (std::string line;std::getline(input,line);) {
      std::istringstream row(line);char kind;unsigned image;std::string one,forward,up;
      check(bool(row>>kind>>image>>one>>forward>>up) && kind=='C' && image==count+1 && image<=images.size(),
            "Incomplete original driver operand record");
      const auto &actual=images[image-1]->sound_driver_startup_operands;
      check(bool(actual),"Verified original driver operands unavailable");
      const auto raw_forward=bytes(forward),raw_up=bytes(up);
      check(raw_forward.size()==12 && raw_up.size()==12 && actual->one_bits==word(one) &&
            std::equal(raw_forward.begin(),raw_forward.end(),actual->basis_forward.begin()) &&
            std::equal(raw_up.begin(),raw_up.end(),actual->basis_up.begin()),
            "Imported driver immutable operand bytes differ");
      if(image==1) {
        const auto &disc=*base->sound_driver_startup_operands;
        check(disc.one_bits==actual->one_bits && disc.basis_forward==actual->basis_forward &&
              disc.basis_up==actual->basis_up,"Imported original disc immutable basis bytes differ");
      }
      ++count;
    }
    check(count==2,"Incomplete original driver image coverage");
    std::cout<<"supportedImages=3 exactImmutableDriverOperandBytes=84 "
                 "mutablePluginEnvironmentSavedSettingsCapsAndStartup=separate\n";
  } catch (const std::exception &error) { std::cerr<<error.what()<<'\n';return 1; }
}
