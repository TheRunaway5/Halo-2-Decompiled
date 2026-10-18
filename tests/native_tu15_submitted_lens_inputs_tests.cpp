#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_lens_flare_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
unsigned checks{};
void check(bool value,const char*why){++checks;if(!value)throw std::runtime_error(why);}
void put(SourceRuntimeMemory&m,unsigned p,unsigned value){std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(8*n));m.write(p,b);}
unsigned word(const SourceRuntimeMemory&m,unsigned p){return Bytes(m.view(p,4)).u32(0);}
std::vector<std::byte> unhex(std::string_view s){check(s.size()%2==0,"odd oracle byte string");std::vector<std::byte>out;for(std::size_t n=0;n<s.size();n+=2)out.push_back(std::byte(std::stoul(std::string(s.substr(n,2)),nullptr,16)));return out;}
std::string hex(std::span<const std::byte>b){std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+="0123456789abcdef"[n>>4];s+="0123456789abcdef"[n&15];}return s;}
nlohmann::json run(SourceRuntimeMemory&m,unsigned entry,std::array<unsigned,8>gp){
 SourcePhysicsWorldRuntimeAbi abi{};abi.gp=gp;abi.eflags=0x202u;put(m,gp[4],0x63000000u);
 std::vector<unsigned>pcs;nlohmann::json stores=nlohmann::json::array();SourcePhysicsWorldRuntimeServices services;
 services.instruction=[&](auto pc){pcs.push_back(pc);};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("whole lens input routine unexpectedly requested supplied owner");};
 source_run_lens_flare_tu15(m,entry,abi,{0x27f,0x1f80},services,[&](const auto&s){stores.push_back({{"pc",s.instruction},{"address",s.address},{"before",hex(s.before)},{"after",hex(s.written)}});});
 return {{"entry_gp",gp},{"exit_gp",abi.gp},{"exit_eflags",abi.eflags},{"pcs",pcs},{"stores",stores}};
}
}
int main(int argc,char**argv)try{
 check(argc<=2,"usage: submitted-lens-inputs-test [ORIGINAL_FIXTURE|--dump]");
 auto m=native_tu15_world_memory({0x60212000u,{},"isolated same factory clock"});const auto cold=m.checkpoint();
 if(argc==2&&std::string_view(argv[1])=="--dump"){
  nlohmann::json j=nlohmann::json::array();for(const auto&r:cold.regions){std::vector<unsigned>bytes;for(auto b:r.bytes)bytes.push_back(std::to_integer<unsigned>(b));j.push_back({{"address",r.address},{"bytes",bytes},{"owner",r.owner},{"access",int(r.access)},{"page_bytes",r.page_bytes},{"committed",r.committed}});}std::cout<<j.dump()<<'\n';return 0;
 }
 for(const auto[p,name]:std::array<std::pair<unsigned,const char*>,2>{{{0x4920b8u,"native TU1.5 cold submitted render-mode word"},{0x4918e8u,"native TU1.5 cold submitted lens-query gate word"}}}){
  const auto found=std::ranges::find(cold.regions,p,&SourceRuntimeMemory::CheckpointRegion::address);check(found!=cold.regions.end()&&found->bytes.size()==4&&found->owner==name&&found->access==SourceRuntimeMemory::Access::ReadWrite&&!found->page_bytes&&found->committed.empty(),"submitted lens input lost exact4-byte loader owner");check(!word(m,p)&&m.native_address(p,4).value>UINT32_MAX,"submitted lens input is prepublished or narrowed");
  for(const auto[at,n]:std::array<std::pair<unsigned,unsigned>,3>{{{p-1,5},{p,5},{p+1,4}}}){bool rejected{};try{m.validate_mutable_view(at,n);}catch(const std::out_of_range&){rejected=true;}check(rejected,"submitted lens input borrowed neighboring bytes");}
 }
 check(m.view(0x491e20u,0x298).size()==0x298&&m.view(0x4920c0u,8).size()==8,"separate existing frame/clock owner changed");check(m.checkpoint()==cold,"ownership rejection mutated cold state");
 constexpr unsigned stack=0x62000000u;m.mount(stack,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,"declared lens caller stack");
 std::array<unsigned,8>gp{0x11u,0x22u,0x33u,0x44u,stack+0x80u,0x55u,0x66u,0x77u};
 put(m,0x4922c0,1);run(m,0x20680,gp);check(Bytes(m.view(0x52dfd8,2)).u16(0)==0x101u&&word(m,0x532094)==0xffffffffu,"whole begin did not publish original ordinary query admission");
 gp[0]=3;run(m,0x20700,gp);check(word(m,0x532094)==3,"whole prepare did not publish actual selected view");put(m,0x4918e8,1);gp[0]=0;run(m,0x20700,gp);check(word(m,0x532094)==3,"authentic nonzero gate admitted another view");
 m.release(stack);m.restore(cold);check(m.checkpoint()==cold,"submitted inputs checkpoint restore differs");
 if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream f(argv[1]);auto oracle=nlohmann::json::parse(f);check(oracle.at("format")=="native_submitted_lens_inputs_tu15_original_v1"&&oracle.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&oracle.at("instruction_patches").empty()&&oracle.at("supplied_callee_returns").empty(),"foreign or patched original authority");
  for(const auto&c:oracle.at("cases")){
   m.restore(cold);m.mount(stack,std::array<std::byte,0x100>{},SourceRuntimeMemory::Access::ReadWrite,"declared lens caller stack");m.write(0x52dfd8,unhex(c.at("query_before").get<std::string>()));put(m,0x4922c0,c.at("stamp").get<int>());put(m,0x4920b8,c.at("mode").get<int>());put(m,0x4918e8,c.at("gate").get<unsigned>());
   const auto native_begin=run(m,0x20680,c.at("begin").at("entry_gp").get<std::array<unsigned,8>>());if(native_begin!=c.at("begin"))throw std::runtime_error("whole20680 differential: "+nlohmann::json::diff(c.at("begin"),native_begin).dump());++checks;check(hex(m.view(0x52dfd8,0x40c0))==c.at("query_after_begin").get<std::string>(),"whole20680 query postimage differs");
   check(run(m,0x20700,c.at("prepare").at("entry_gp").get<std::array<unsigned,8>>())==c.at("prepare"),"whole20700 ABI/trace/store differential differs");check(hex(m.view(0x52dfd8,0x40c0))==c.at("query_after_prepare").get<std::string>(),"whole20700 query postimage differs");check(word(m,0x4920b8)==unsigned(c.at("mode").get<int>())&&word(m,0x4918e8)==c.at("gate").get<unsigned>(),"lens calls mutated submitted inputs");m.release(stack);
  }
 }
 std::cout<<"PASS "<<checks<<" checks: exact two4-byte cold submitted lens inputs; whole20680/20700 admission, gate and checkpoint controls"<<(argc==2?"; 16 unchanged-original GP/flag/trace/store/full-query differential cases":"")<<"\nLIMIT no completed GPU query/frame or visual parity claim\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
