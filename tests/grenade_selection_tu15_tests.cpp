#include "halo2/inventory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)throw FormatError("Supply original TU1.5 selector fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;unsigned cases{};
  for(const auto &row:fixture.at("cases")) {
    const std::array<std::uint8_t,2> counts{row.at("counts")[0],row.at("counts")[1]};
    const auto actual=grenade_ready_type(row.at("current"),counts,row.at("direction"));
    if(int(actual)!=row.at("result"))throw FormatError("Original grenade selection differs: "+row.dump());
    ++cases;
  }
  if(cases!=735)throw FormatError("Original grenade selection cases were omitted");
  std::cout<<"PASS "<<cases<<" original TU1.5 grenade selection cases\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
