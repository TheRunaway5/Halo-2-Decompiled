#include "halo2/object_lighting.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv)try {
  if(argc!=2)throw FormatError("Supply original TU1.5 lighting direction fixture");
  nlohmann::json fixture;std::ifstream(argv[1])>>fixture;
  if(fixture.at("xbe_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" || !fixture.at("service_replacements").empty())throw FormatError("Invalid lighting direction authority");
  unsigned count=0,mismatches=0;
  for(const auto& c:fixture.at("cases")) {
    const auto f=[](const auto& x){return std::bit_cast<float>(x.template get<std::uint32_t>());};
    const auto v=source_object_light_direction({f(c["input"][0]),f(c["input"][1]),f(c["input"][2])},f(c["angle"]));
    const std::array actual{std::bit_cast<std::uint32_t>(v.x),std::bit_cast<std::uint32_t>(v.y),std::bit_cast<std::uint32_t>(v.z)};
    const auto expected=c["expected"].get<std::array<std::uint32_t,3>>();
    if(actual!=expected){if(mismatches<4)std::cerr<<"case "<<count<<" expected "<<c["expected"]<<" actual "<<nlohmann::json(actual)<<'\n';++mismatches;}
    ++count;
  }
  if(mismatches)throw FormatError(std::to_string(mismatches)+" original lighting direction cases differ");
  std::cout<<"PASS "<<count<<" bit-exact unchanged TU1.5 lighting direction cases\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
