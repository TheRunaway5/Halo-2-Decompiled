#include "halo2/unit_input.hpp"
#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <vector>
#include <stdexcept>

namespace {
void check(bool value,const std::string &context) {
  if(!value)throw std::runtime_error("Original unit input differs: "+context);
}
std::vector<std::byte> bytes(const std::string &text) {
  check(text.size()%2==0,"hex extent");std::vector<std::byte> out;
  for(unsigned i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
  return out;
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=2)throw std::runtime_error("Supply immutable-source unit input fixture export");
    if(!std::filesystem::exists(argv[1]))return 77;
    std::ifstream file(argv[1]);unsigned inputs=0,slots=0;
    for(std::string line;std::getline(file,line);) {
      std::istringstream stream(line);std::vector<std::string> r;
      for(std::string s;stream>>s;)r.push_back(s);if(r.empty())continue;
      const auto context="case "+r.at(1)+" call "+r.at(2);
      if(r[0]=="I") {
        auto unit=bytes(r.at(4));const auto expected=bytes(r.at(5)),captured=bytes(r.at(3));
        check(captured.size()==0x7c && unit.size()==0x800 && expected.size()==unit.size(),context+" source storage extents");
        std::array<std::byte,0x7c> input;std::copy(captured.begin(),captured.end(),input.begin());
        halo2::apply_source_unit_input(unit,input);
        check(unit==expected,context+" complete unit prefix, including untouched bytes");++inputs;
      }else if(r[0]=="S") {
        const auto unit=bytes(r.at(3));check(unit.size()==0x800,context+" slot storage extent");
        check(std::uint16_t(halo2::source_inventory_empty_slot(unit))==std::stoul(r.at(4),nullptr,16),context+" defined AX");++slots;
      }else throw std::runtime_error("Unknown unit input fixture row");
    }
    check(inputs==10 && slots==9,"complete independently observed suite");
    std::cout<<inputs<<" original 0x7C-byte unit inputs/full prefixes and "<<slots<<" ascending full-datum slot scans matched\n";
  }catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
