#include "halo2/native_tu15_island_integration.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace {
using namespace halo2;using J=nlohmann::json;
void check(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
std::vector<std::byte> bytes(const std::string&s){check(s.size()%2==0,"Invalid island bytes");std::vector<std::byte>b;for(std::size_t n=0;n<s.size();n+=2)b.push_back(std::byte(std::stoul(s.substr(n,2),nullptr,16)));return b;}
std::string hex(std::span<const std::byte>b){const char*d="0123456789abcdef";std::string s;for(auto b:b){auto n=std::to_integer<unsigned>(b);s+=d[n>>4];s+=d[n&15];}return s;}
struct Memory:SourcePhysicsContactMemory{
  std::map<std::uint32_t,std::vector<std::byte>>banks;
  auto at(std::uint32_t p,std::size_t n)const{auto i=banks.upper_bound(p);check(i!=banks.begin(),"Missing island bank");--i;check(std::uint64_t(p-i->first)+n<=i->second.size(),"Missing island extent");return i;}
  void read(std::uint32_t p,std::span<std::byte>b)const override{auto i=at(p,b.size());std::copy_n(i->second.begin()+p-i->first,b.size(),b.begin());}
  void write(std::uint32_t p,std::span<const std::byte>b)override{auto i=at(p,b.size());std::copy(b.begin(),b.end(),banks.at(i->first).begin()+p-i->first);}
};
struct Stop{std::string owner;std::vector<std::uint32_t>arguments;};
}
int main(int argc,char**argv){try{
  check(argc==2&&std::filesystem::exists(argv[1]),"Supply required independent original island fixture");std::ifstream in(argv[1]);const auto f=J::parse(in);
  check(f.at("image_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&f.at("island_sites")==98,"Island TU1.5 source authority changed");
  check(f.at("required_backend")=="unchanged-tu15-island-unicorn-stores+physical-i386-banks-fp-v1","Missing required original island backend");
  std::size_t cases=0,writes=0,forces=0;
  for(const auto&c:f.at("cases")){
    const auto name=c.at("name").get<std::string>();check(c.at("physical_verified").get<bool>(),"Island physical original not verified");
    Memory m;for(const auto&b:c.at("banks"))m.banks.emplace(b.at("address"),bytes(b.at("before")));
    std::vector<SourcePhysicsContactRecordStore>stores;NativeTu15IslandIntegrationMemory storage(m,[&](const auto&s){stores.push_back(s);});
    NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};NativeTu15IslandIntegrationServices services;
    services.integrate_pose=[](auto motion,auto owner,auto step,auto&){throw Stop{"pose",{motion.identity,owner.identity,step.identity}};};
    services.solve_constraints=[](const auto&i,auto&){throw Stop{"constraints",{i.step.identity,i.solver_configuration,i.jacobian_context,i.gravity.identity,i.island.identity,i.bodies.identity,i.body_count,i.constraints,i.constraint_count}};};
    services.update_action=[](auto,auto,auto,auto&){throw std::runtime_error("Unexpected unqualified action fixture");};
    services.apply_other_motion_forces=[](auto,auto,auto,auto,auto&){throw std::runtime_error("Unexpected unqualified motion kind");};
    NativeTu15IslandIntegrationStats stats;bool stopped=false;Stop stop{};
    try{native_tu15_integrate_physics_island(storage,{c.at("island")},{c.at("configuration")},fp,0,services,&stats);}
    catch(const Stop&s){stopped=true;stop=s;}
    check(stopped==!c.at("boundary").is_null(),name+" island completion differs");
    if(stopped)check(stop.owner==c.at("boundary").at("owner").get<std::string>()&&stop.arguments==c.at("boundary").at("arguments").get<std::vector<std::uint32_t>>(),name+" real island boundary differs");
    const auto&expected=c.at("writes");check(stores.size()==expected.size(),name+" island store count differs");
    for(std::size_t n=0;n<stores.size();++n){const auto&a=stores[n];const auto&b=expected[n];check(a.instruction==b.at("instruction")&&a.address==b.at("address")&&hex(a.before)==b.at("before").get<std::string>()&&hex(a.written)==b.at("written").get<std::string>(),name+" ordered island store differs at "+std::to_string(n));}
    for(const auto&b:c.at("banks"))check(hex(m.banks.at(b.at("address")))==b.at("after").get<std::string>(),name+" retained island bytes differ");
    const auto&end=c.at("after_fp");check(fp.control_word==end[0]&&fp.status==end[1]&&fp.mxcsr==end[2],name+" island complete FP differs");
    check(stats.islands==1&&stats.native_force_calls==c.at("native_force_calls"),name+" direct native force not executed");
    ++cases;writes+=stores.size();forces+=stats.native_force_calls;
  }
  check(cases>0,"Empty island original fixture");std::cout<<cases<<" original island prefix/completion cases, "<<writes<<" ordered stores, "<<forces<<" direct native force invocations passed; residual pose/constraints/full world remain open\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
