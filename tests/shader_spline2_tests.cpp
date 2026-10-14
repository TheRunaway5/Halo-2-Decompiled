#include "halo2/cache.hpp"
#include "halo2/shader_animation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=2)return 2;
  std::ifstream file(argv[1]);if(!file)return 77;
  const ShaderFunctionTables tables;std::string line;unsigned cases{};
  while(std::getline(file,line)) {
    if(line.empty() || line[0]=='#')continue;
    std::istringstream row(line);std::string hex;std::uint32_t input,range,expected;
    if(!(row>>hex>>std::hex>>input>>range>>expected) || hex.size()%2)throw FormatError("Truncated original spline2 case");
    std::vector<std::byte> curve;
    for(unsigned at=0;at<hex.size();at+=2)curve.push_back(std::byte(std::stoul(hex.substr(at,2),nullptr,16)));
    const auto actual=std::bit_cast<std::uint32_t>(shader_function_fraction(curve,
        std::bit_cast<float>(input),std::bit_cast<float>(range),tables));
    if(actual!=expected) {
      std::cerr<<"case="<<cases<<" input="<<std::hex<<input<<" range="<<range<<" expected="<<expected<<" actual="<<actual<<'\n';
      throw FormatError("Native spline2 differs from original instructions on this host x86");
    }
    ++cases;
  }
  if(cases<100)throw FormatError("Original spline2 coverage missing");
  std::cout<<"PASS "<<cases<<" original host-x86 spline2 samples; Xbox RSQRTSS precision unverified\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
