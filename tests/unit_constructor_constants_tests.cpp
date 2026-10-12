#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::vector<std::byte> raw(const std::string &s){
  check(!s.empty() && s.size()%2==0,"Incomplete original operand");
  std::vector<std::byte> bytes(s.size()/2);
  for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(std::stoul(s.substr(i*2,2),nullptr,16));
  return bytes;
}
std::uint32_t word(const std::string &s){
  const auto bytes=raw(s);check(bytes.size()==4,"Incomplete original scalar");return halo2::Bytes(bytes).u32(0);
}
}
int main(int argc,char **argv){
  try{
    check(argc==5,"Supply original operands and base/1.1/u5 images");
    for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
    check(!halo2::OriginalData::load_xbe(argv[2])->unit_constructor_tail_constants,
          "Unverified base constructor operands invented");
    std::ifstream input(argv[1]);std::size_t images=0;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;unsigned image;std::string one,zero,normal,special,pairs;
      check(bool(row>>kind>>image>>one>>zero>>normal>>special>>pairs) && kind=='C' && image==images+1 && image<=2,
            "Incomplete or unordered original operands");
      const auto owned=halo2::OriginalData::load_xbe(argv[image+2]);
      check(bool(owned->unit_constructor_tail_constants),"Verified constructor operands unavailable");
      const auto &actual=*owned->unit_constructor_tail_constants;
      check(actual.one_bits==word(one) && actual.zero_bits==word(zero) &&
            actual.normal_blend_bits==word(normal) && actual.special_blend_bits==word(special),
            "Original scalar constructor operands differ");
      const auto bytes=raw(pairs);check(bytes.size()==32,"Incomplete original activity pairs");
      for(std::size_t r=0;r<4;++r)for(std::size_t c=0;c<2;++c)
        check(actual.activity_pairs[r][c]==halo2::Bytes(bytes).u32(r*8+c*4),"Full original activity bank differs");
      ++images;
    }
    check(images==2,"Missing verified update operands");
    std::cout<<"supportedImages=2 scalarWords=8 activityPairs=8 exactBytes=96 unavailableBase=passed liveConstructorState=absent\n";
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
