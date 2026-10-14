#include "halo2/physics_live_support_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
using namespace halo2;using J=nlohmann::json;
struct Memory:SourcePhysicsContactMemory {
 std::map<std::uint32_t,std::vector<std::byte>> banks;
 auto at(std::uint32_t p)const {auto i=banks.upper_bound(p);if(i==banks.begin())throw std::runtime_error("missing bank");--i;if(p-i->first>=i->second.size())throw std::runtime_error("uncommitted address");return i;}
 void read(std::uint32_t p,std::span<std::byte>out)const override {for(auto &v:out){auto i=at(p);v=i->second[p++-i->first];}}
 void write(std::uint32_t p,std::span<const std::byte>in)override {for(auto v:in){auto i=at(p);banks.at(i->first)[p++-i->first]=v;}}
 void put(std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;std::memcpy(b.data(),&v,4);write(p,b);}
};
J replay(const J &c) {
 Memory m;m.banks[0x4ecc14]=std::vector<std::byte>(4);m.banks[0x2100000]=std::vector<std::byte>(0x100000);
 constexpr unsigned scene=0x2100000,block=0x2101000,rows=0x2102000,input=0x2104000,stack=0x2108000,stop=0x3000000;
 m.put(0x4ecc14,c["present"].get<bool>()?scene:0);m.put(scene+0x80,c["active"]);m.put(scene+0x84,block);m.put(block+0x58,c["rows"].size());m.put(block+0x5c,rows);
 for(unsigned n=0;n<c["rows"].size();++n)for(unsigned j=0;j<3;++j)m.put(rows+n*12+j*4,c["rows"][n][j]);
 m.put(input,c["input"][0]);m.put(input+4,c["input"][1]);m.put(stack,stop);m.put(stack+4,input);m.put(stack+8,c["parameter"]);
 SourcePhysicsWorldRuntimeAbi a{};a.gp=c["gp"].get<decltype(a.gp)>();a.gp[4]=stack;a.eflags=2;SourcePhysicsWorldRuntimeServices s;J trace=J::array(),writes=J::array();s.instruction=[&](auto p){trace.push_back(p);};s.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("unexpected service");};
 source_run_physics_live_support_tu15(m,0x10f960,a,{0x37f,0x1f80},s,[&](const auto&w){unsigned value=0;for(unsigned j=0;j<w.written.size();++j)value|=std::to_integer<unsigned>(w.written[j])<<(j*8);writes.push_back(J::array({w.address,w.written.size(),value}));});
 return J({{"gp",a.gp},{"eflags",a.eflags},{"instructions",trace},{"writes",writes}});
}
int main(int argc,char **argv) {try {
 std::ifstream fixture;std::istream *input=&std::cin;if(argc==2){fixture.open(argv[1]);if(!fixture)throw std::runtime_error("Missing original scenery lightmap fixture");input=&fixture;}
 std::string line;unsigned cases=0;while(std::getline(*input,line)) {
  const auto c=J::parse(line);const auto got=replay(argc==2?c.at("input"):c);
  if(argc==2){if(got!=c.at("original"))throw std::runtime_error("Original scenery lightmap mismatch at case "+std::to_string(cases));}
  else std::cout<<got.dump()<<'\n';++cases;
 }
 if(argc==2){if(cases!=512)throw std::runtime_error("Incomplete scenery lightmap fixtures");std::cout<<"PASS 512 complete original scenery lightmap lookups\n";}
 }catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}

