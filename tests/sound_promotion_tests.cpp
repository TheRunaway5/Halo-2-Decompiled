#include "halo2/sound_promotion.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

namespace {
std::uint32_t u(const std::string &s) {return std::uint32_t(std::stoul(s,nullptr,16));}
void require(bool x,const char *s) {if(!x)throw std::runtime_error(s);}
std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> out;std::istringstream f(line);std::string q;
  while(std::getline(f,q,'\t'))out.push_back(q);
  return out;
}
std::vector<std::byte> decode(const std::string &s) {
  require(!(s.size()%2),"Invalid source byte extent");std::vector<std::byte> b(s.size()/2);
  for(std::size_t i=0;i<b.size();++i)b[i]=std::byte(u(s.substr(i*2,2)));
  return b;
}
struct Region {std::uint32_t address;std::vector<std::byte> expected;};
}
int main(int argc,char **argv)try {
  require(argc==2,"Usage: sound_promotion_tests <fixtures.tsv>");
#if defined(__SSE__)
  const auto old_mxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
#endif
  std::ifstream f(argv[1]);require(bool(f),"Original promotion fixtures unavailable");
  std::string line;require(bool(std::getline(f,line))&&line=="SOUND_PROMOTION_NATIVE_V1","Wrong original fixture schema");
  halo2::SoundPromotionState state{};halo2::SoundPromotionConstants constants{};
  halo2::SoundPromotionServices services;
  std::unique_ptr<halo2::NativeSoundStorage> storage;
  std::vector<Region> regions;
  std::vector<halo2::SoundStartupStore> expected;
  bool active=false,initialized=false,clock=false;
  std::uint32_t sound=0,tick=0,result=0,selector=0;
  std::size_t calls=0,writes=0,maps=0,suppressed=0,no_promotion=0,positive=0,clock_calls=0,bytes_checked=0;
  auto initialize=[&] {
    storage=std::make_unique<halo2::NativeSoundStorage>(halo2::NativeSoundStorageConfiguration{0x480d68,0xb000000,0xc000000});
    std::vector<std::pair<std::uint64_t,std::uint64_t>> ranges;
    for(const auto &r:regions)ranges.emplace_back(r.address&~std::uint32_t(4095),(std::uint64_t(r.address)+r.expected.size()+4095)&~std::uint64_t(4095));
    std::ranges::sort(ranges);std::uint64_t start=0,end=0;
    for(const auto &[a,b]:ranges) {
      if(a<=end) {end=std::max(end,b);continue;}
      if(end)storage->map_zeroed(std::uint32_t(start),std::size_t(end-start));
      start=a;end=b;
    }
    if(end)storage->map_zeroed(std::uint32_t(start),std::size_t(end-start));
    for(const auto &r:regions)std::ranges::copy(r.expected,storage->memory(r.address,r.expected.size()).begin());
    services.memory=[&](auto at,auto size) {
      // Padded OS pages do not implicitly become additional source fields.
      require(std::ranges::any_of(regions,[&](const auto &r){return at>=r.address&&std::uint64_t(at)+size<=std::uint64_t(r.address)+r.expected.size();}),"Native owner read unprovided source bytes");
      return storage->memory(at,size);
    };
    services.tick_milliseconds=[&]{++clock_calls;return tick;};
    initialized=true;
  };
  while(std::getline(f,line)) {
    const auto q=fields(line);if(q.empty())continue;
    if(q[0]=="K") {require(q.size()==4,"Source constants");constants={u(q[1]),u(q[2]),u(q[3])};}
    else if(q[0]=="M") {
      require(!active&&q.size()==7,"Source map state");regions.clear();storage.reset();initialized=false;
      state={u(q[3]),u(q[4]),u(q[5]),u(q[6])};
    }else if(q[0]=="R") {require(!initialized&&q.size()==3,"Literal source region");regions.push_back({u(q[1]),decode(q[2])});}
    else if(q[0]=="C") {
      require(!active&&q.size()==6,"Source call");if(!initialized)initialize();active=true;expected.clear();
      clock=q[1]=="clock";sound=u(q[2]);tick=u(q[3]);result=u(q[4]);selector=u(q[5]);
    }else if(q[0]=="S") {
      require(active&&q.size()==6,"Original ordered store");expected.push_back({u(q[1]),u(q[2]),unsigned(std::stoul(q[3])),u(q[4]),u(q[5])});
    }else if(q[0]=="E") {
      require(active,"Call terminator");active=false;
      for(const auto &s:expected) {
        auto it=std::ranges::find_if(regions,[&](const auto &r){return s.address>=r.address&&std::uint64_t(s.address)+s.bytes<=std::uint64_t(r.address)+r.expected.size();});
        require(it!=regions.end(),"Original store outside literal source region");
        const auto offset=s.address-it->address;std::uint32_t before=0;
        for(unsigned i=0;i<s.bytes;++i)before|=std::to_integer<std::uint32_t>(it->expected[offset+i])<<(i*8);
        require(before==s.before,"Original reconstruction before bits");
        for(unsigned i=0;i<s.bytes;++i)it->expected[offset+i]=std::byte(s.after>>(i*8));
      }
      std::vector<halo2::SoundStartupStore> actual;std::int32_t returned=0;
      const auto observer=[&](const auto &s){actual.push_back(s);};
      if(clock)halo2::source_sound_promotion_clock(state,constants,services,observer);
      else returned=halo2::source_sound_promote(state,sound,0xa400000,constants,services,observer);
      if(actual!=expected||(!clock&&std::uint32_t(returned)!=result)) {
        std::cerr<<"call "<<calls<<" sound "<<std::hex<<sound<<std::dec<<" expected/actual stores "<<expected.size()<<"/"<<actual.size()<<"\n";
        for(std::size_t i=0;i<std::min(expected.size(),actual.size());++i)if(expected[i]!=actual[i]) {std::cerr<<"first store "<<i<<" instruction "<<std::hex<<actual[i].instruction<<"/"<<expected[i].instruction<<" bits "<<actual[i].after<<"/"<<expected[i].after<<std::dec<<"\n";break;}
        throw std::runtime_error("Complete original promotion/clock mismatch");
      }
      if(!clock) {
        auto output=storage->memory(0xa400000,4);std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(output[i])<<(i*8);
        require(value==selector,"Original numeric selector mismatch");
        suppressed+=result==1;no_promotion+=selector==0xffffffffu;positive+=selector!=0xffffffffu;
      }
      ++calls;writes+=actual.size();
    }else if(q[0]=="ENDMAP") {
      require(!active&&initialized,"Incomplete source map");
      for(const auto &r:regions) {require(std::ranges::equal(r.expected,storage->memory(r.address,r.expected.size())),"Complete original region mismatch");bytes_checked+=r.expected.size();}
      ++maps;
    }else throw std::runtime_error("Unknown original fixture row");
  }
  require(!active&&maps==46&&calls==74318&&writes==249740,"Incomplete original source suite");
  // Missing dependencies must stop before producing source success/state.
  const auto unavailable=halo2::SoundPromotionServices{};bool stopped=false;
  try {halo2::source_sound_promote(state,sound,0xa400000,constants,unavailable);}catch(const std::runtime_error &){stopped=true;}
  require(stopped,"Missing memory silently admitted");stopped=false;
  try {halo2::source_sound_promotion_clock(state,constants,unavailable);}catch(const std::runtime_error &){stopped=true;}
  require(stopped,"Missing real clock silently admitted");
#if defined(__SSE__)
  _mm_setcsr(old_mxcsr);
#endif
  std::cout<<"PASS complete promotion/clock "<<calls<<" calls, "<<writes<<" ordered stores, "<<maps<<" image/map banks, "<<clock_calls<<" original clock consumers, "<<positive<<" numeric selectors, "<<suppressed<<" suppressed, "<<no_promotion<<" NONE, "<<bytes_checked<<" full region bytes; no counter/selector/clock-state/admission results supplied.\n";
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
