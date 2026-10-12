#include "halo2/vista_session_player_preparation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace halo2;
namespace {
void check(bool value,const std::string& message) {if(!value)throw std::runtime_error(message);}
std::vector<std::byte> pattern(unsigned seed) {
  std::vector<std::byte> b(vista_session_source_size+32,std::byte{0xa5});
  for(std::size_t i=0;i<vista_session_source_size;++i)b[i]=std::byte((i*37+(i>>8)*17+seed)&255);
  return b;
}
void patches(std::vector<std::byte>& b,const std::string& text) {
  if(text=="-")return;
  std::istringstream input(text);std::string piece;
  while(std::getline(input,piece,',')) {
    const auto colon=piece.find(':');check(colon!=std::string::npos,"invalid original patch");
    const auto offset=std::stoul(piece.substr(0,colon),nullptr,16);const auto hex=piece.substr(colon+1);
    check(!hex.empty() && hex.size()%2==0 && offset+hex.size()/2<=vista_session_source_size,"unowned original patch extent");
    for(std::size_t i=0;i<hex.size()/2;++i)b[offset+i]=std::byte(std::stoul(hex.substr(i*2,2),nullptr,16));
  }
}
void put(std::vector<std::byte>& b,unsigned o,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[o+i]=std::byte(v>>(8*i));
}
template<class F> void rejected(F&& f,const std::string& why) {
  try {f();}catch(const FormatError&){return;}
  throw std::runtime_error("Unsupported preparation boundary admitted: "+why);
}
void boundaries() {
  auto b=pattern(19);put(b,0x73a0,5);put(b,0x5101,0,1);put(b,0x84,19);put(b,0x1258,0);
  auto before=b;rejected([&]{vista_session_prepare_players(b);},"scratch peer overflow");check(b==before,"peer rejection mutated owner");
  put(b,0x84,1);put(b,0x1258,1);put(b,0x125c+0x14,0);put(b,0x125c+8,18);
  before=b;rejected([&]{vista_session_prepare_players(b);},"row references unowned peer address");check(b==before,"row rejection mutated owner");
  put(b,0x84,2);before=b;
  rejected([&]{vista_session_refresh_parameters(b,-1,0);},"missing consumed network clock");check(b==before,"clock rejection mutated owner");
  put(b,0x73a0,7);before=b;
  check(!vista_session_refresh_parameters(b,-1,0).accepted && b==before,"inactive refresh required unconsumed clock");
  put(b,0x73a0,5);put(b,0x84,1);before=b;
  const auto r=vista_session_refresh_parameters(b,-1,0);
  check(r.accepted && !r.clock_consumed && b==before,"one-machine refresh manufactured a transition");
  auto short_bank=std::span(b).first(vista_session_source_size-1);
  rejected([&]{vista_session_prepare_players(short_bank);},"short preparation bank");
  rejected([&]{vista_session_refresh_parameters(short_bank,-1,0,1);},"short refresh bank");
  // The source early prepared branch does not inspect invalid downstream rows.
  put(b,0x5101,0x80,1);put(b,0x84,0x7fffffffu);before=b;
  check(vista_session_prepare_players(b) && b==before,"existing preparation read unconsumed rows");
}
} // namespace
int main(int argc,char** argv) {
  try {
    check(argc==2,"provide original fixture path");std::ifstream file(argv[1]);check(bool(file),"fixture open failed");
    std::string line;unsigned count{};
    while(std::getline(file,line)) {
      if(line.empty() || line[0]=='#')continue;
      std::istringstream row(line);char kind{};std::string label,before_patch,after_patch;
      unsigned seed{},flag{},al{},clocks{};std::uint32_t target{},clock{};
      row>>kind>>label>>std::hex>>seed>>target>>flag>>clock>>al>>clocks>>before_patch>>after_patch;
      check(bool(row) && flag<=255 && al<=255 && clocks<=1,label+": invalid fixture");
      auto before=pattern(seed);patches(before,before_patch);auto expected=before;patches(expected,after_patch);auto actual=before;
      if(kind=='P') {
        check(vista_session_prepare_players(actual)==(al!=0),label+": original preparation AL differs");check(clocks==0,label+": unexpected preparation clock");
      } else {
        check(kind=='R',label+": invalid fixture kind");
        const auto r=vista_session_refresh_parameters(actual,std::bit_cast<std::int32_t>(target),std::uint8_t(flag),clock);
        check(r.accepted==(al!=0) && r.clock_consumed==(clocks!=0),label+": original refresh AL/clock consumption differs");
      }
      if(actual!=expected) {
        const auto at=std::mismatch(actual.begin(),actual.end(),expected.begin()).first-actual.begin();
        throw std::runtime_error(label+": original complete owner bank differs at offset "+std::to_string(at));
      }
      ++count;
    }
    check(count==2384,"original preparation fixture count changed");boundaries();
    std::cout<<"PASS "<<count<<" original complete Vista player-preparation/refresh banks; atomic boundaries and suffix preservation\n";
    return 0;
  } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
