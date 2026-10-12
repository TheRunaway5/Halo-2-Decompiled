#include "halo2/sound_submit.hpp"
#include "halo2/native_sound_storage.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::uint32_t u(const std::string &s) {return std::uint32_t(std::stoul(s,nullptr,16));}
std::vector<std::string> fields(const std::string &line) {
  std::vector<std::string> out;std::istringstream f(line);std::string q;
  while(std::getline(f,q,'\t'))out.push_back(q);
  return out;
}
std::vector<std::byte> decode(const std::string &s) {
  if(s.size()%2)throw std::runtime_error("Invalid literal byte extent");
  std::vector<std::byte> out(s.size()/2);
  for(std::size_t i=0;i<out.size();++i)out[i]=std::byte(u(s.substr(i*2,2)));
  return out;
}
void require(bool pass,const char *why) {if(!pass)throw std::runtime_error(why);}
void apply(std::span<std::byte> b,const halo2::SoundStartupStore &s,std::uint32_t base) {
  const auto at=s.address-base;require(std::uint64_t(at)+s.bytes<=b.size(),"Source store bounds");
  std::uint32_t before=0;for(unsigned i=0;i<s.bytes;++i)before|=std::to_integer<std::uint32_t>(b[at+i])<<(8*i);
  require(before==s.before,"Source reconstruction before bits");
  for(unsigned i=0;i<s.bytes;++i)b[at+i]=std::byte(s.after>>(8*i));
}
}
int main(int argc,char **argv)try {
  require(argc==3,"Usage: sound_submit_tests <fixtures.tsv> <literal-lockout-regions.bin>");
  std::ifstream fixture(argv[1]);require(bool(fixture),"Original fixtures unavailable");
  std::ifstream literal_file(argv[2],std::ios::binary);require(bool(literal_file),"Original literal map memory unavailable");
  std::vector<char> literal_char((std::istreambuf_iterator<char>(literal_file)),{});
  const auto literal=std::as_bytes(std::span(literal_char));
  halo2::NativeSoundStorage storage({0x480d68,0xb000000,0xc000000});
  constexpr std::uint32_t arena=0xa000000,request=0xa400000;
  storage.map_zeroed(arena,0x23000);storage.map_zeroed(request,4096);
  halo2::SoundSubmitState state{};halo2::SoundSubmitConstants constants{};
  halo2::SoundSubmitServices services;services.memory=[&](auto address,auto size){return storage.memory(address,size);};
  std::vector<std::byte> initial,expected_arena,expected_request;std::vector<halo2::SoundStartupStore> expected;
  std::uint32_t sound=0,result=0;std::size_t cases=0,writes=0,effect_cases=0;bool active=false;
  std::string line;require(bool(std::getline(fixture,line))&&line=="SOUND_SUBMIT_NATIVE_V1","Wrong source fixture format");
  while(std::getline(fixture,line)) {
    const auto f=fields(line);if(f.empty())continue;
    if(f[0]=="LITERAL") {
      require(f.size()==5,"Literal descriptor fields");const auto base=u(f[2]);const auto table_size=std::stoull(f[3]),meta_size=std::stoull(f[4]);
      require(table_size+meta_size==literal.size(),"Literal serialized regions extent");
      storage.map_zeroed(0x1080000,((table_size+4095)/4096)*4096);
      storage.map_zeroed(base,meta_size);
      std::ranges::copy(literal.first(table_size),storage.memory(0x1080000,table_size).begin());
      std::ranges::copy(literal.subspan(table_size),storage.memory(base,meta_size).begin());
    } else if(f[0]=="IMAGE") {
      require(f.size()==11,"Image/state fields");constants.effect_callbacks=u(f[2]);
      state={u(f[3]),u(f[4]),u(f[5]),u(f[6]),u(f[7]),u(f[8]),u(f[9]),u(f[10])};
    } else if(f[0]=="ARENA") {
      initial=decode(f[1]);require(initial.size()==0x23000,"Source arena extent");
    } else if(f[0]=="CASE") {
      require(!active&&f.size()==5,"Original case fields");active=true;sound=u(f[1]);result=u(f[4]);expected.clear();
      const auto before=decode(f[2]);expected_request=decode(f[3]);expected_arena=initial;
      require(before.size()==160&&expected_request.size()==160,"Original raw request extent");
      std::ranges::copy(initial,storage.memory(arena,initial.size()).begin());
      std::ranges::copy(before,storage.memory(request,160).begin());
    } else if(f[0]=="STORE") {
      require(active&&f.size()==6,"Original store fields");expected.push_back({u(f[1]),u(f[2]),unsigned(std::stoul(f[3])),u(f[4]),u(f[5])});
    } else if(f[0]=="END") {
      require(active,"Case terminator without source case");active=false;
      for(const auto &store:expected)if(arena<=store.address&&store.address<arena+initial.size())apply(expected_arena,store,arena);
      std::vector<halo2::SoundStartupStore> actual;
      const auto returned=halo2::source_sound_submit(state,sound,request,constants,services,[&](const auto &q){actual.push_back(q);});
      if(returned!=result||actual!=expected) {
        std::cerr<<"Case "<<cases<<" sound "<<std::hex<<sound<<std::dec<<" expectedstores "<<expected.size()<<" actual "<<actual.size()<<"\n";
        for(std::size_t i=0;i<std::min(actual.size(),expected.size());++i)if(actual[i]!=expected[i]) {
          std::cerr<<"firststore "<<i<<" PC "<<std::hex<<actual[i].instruction<<"/"<<expected[i].instruction<<" address "<<actual[i].address<<"/"<<expected[i].address<<" before "<<actual[i].before<<"/"<<expected[i].before<<" after "<<actual[i].after<<"/"<<expected[i].after<<std::dec<<"\n";break;
        }
        throw std::runtime_error("Original complete submit return/store mismatch");
      }
      require(std::ranges::equal(expected_arena,storage.memory(arena,initial.size())),"Original complete arena mismatch");
      require(std::ranges::equal(expected_request,storage.memory(request,160)),"Original complete raw160 request mismatch");
      writes+=actual.size();effect_cases+=!actual.empty();++cases;
    } else throw std::runtime_error("Unknown source fixture record");
  }
  require(!active&&cases==2574,"Incomplete original source suite");
  std::cout<<"PASS complete raw submit "<<cases<<" cases, "<<writes<<" ordered stores, "<<effect_cases<<" actual allocate/bind/reject/release cases; real NativeSoundStorage, literal serialized map memory, no ready/admission/native device/voice results.\n";
}catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
