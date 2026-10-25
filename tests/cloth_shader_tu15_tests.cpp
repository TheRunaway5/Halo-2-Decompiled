#include "physics/cloth_native/shader.hpp"
#include <nlohmann/json.hpp>
#include <map>
#include <fstream>
#include <iostream>
using namespace halo2;
struct Memory final:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::byte> bytes;
 void read(std::uint32_t at,std::span<std::byte> out)const override {for(auto& b:out)b=bytes.at(at++);}
 void write(std::uint32_t at,std::span<const std::byte> in)override {for(auto b:in)bytes[at++]=b;}
 void word(std::uint32_t p,std::uint32_t v){for(unsigned i=0;i<4;++i)bytes[p+i]=std::byte(v>>(i*8));}
};
struct State {
 Memory& memory;std::array<std::uint32_t,8> gp;
 physics_contact_runtime_detail::Flags flags;
 std::uint32_t pop(){const auto v=physics_world_runtime_detail::r(memory,gp[4],4);gp[4]+=4;return v;}
};
int main(int argc,char**argv)try {
 if(argc!=2)throw std::runtime_error("Supply original cloth getter cases");nlohmann::json j;std::ifstream(argv[1])>>j;
 if(j.at("original_sha256")!="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" || !j.at("supplied_service_results").empty())throw std::runtime_error("Unpinned cloth reference");
 unsigned cases{};
 for(const auto& row:j.at("cases")) {
  Memory m;State s{m,row.at("gp").get<std::array<std::uint32_t,8>>()};
  const auto slot=row.at("slot").get<unsigned>(),index=row.at("index").get<unsigned>(),flags=row.at("flags").get<unsigned>();
  s.flags.cf=flags&1;s.flags.pf=flags&4;s.flags.af=flags&16;s.flags.zf=flags&64;s.flags.sf=flags&128;s.flags.of=flags&2048;
  m.word(s.gp[4],0x10000);m.word(s.gp[4]+4,0xcafe0000|slot);m.word(0x4ecc08,0x4001000);m.word(0x4001044,0x4002000);m.word(0x4f0414,0x4060000);
  m.word(0x4002004+slot*0x1258,0xbeef0000|index);m.word(0x4060008+index*16,0x4065000);m.word(0x406500c,0xdada0000|index);
  const auto before=m.bytes;cloth_native_tu15_detail::shader(s);
  const unsigned result=(unsigned(s.flags.cf))|(unsigned(s.flags.pf)<<2)|(unsigned(s.flags.af)<<4)|(unsigned(s.flags.zf)<<6)|(unsigned(s.flags.sf)<<7)|(unsigned(s.flags.of)<<11);
  if(s.gp!=row.at("result").get<decltype(s.gp)>() || (result&0x8c5)!=(row.at("result_flags").get<unsigned>()&0x8c5) || m.bytes!=before)throw std::runtime_error("Original cloth shader getter register/flags/read-only contract differs");
  ++cases;
 }
 if(cases!=32)throw std::runtime_error("Incomplete cloth getter reference");std::cout<<"PASS32 original cloth shader getter ABI/flags/read-only cases\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
