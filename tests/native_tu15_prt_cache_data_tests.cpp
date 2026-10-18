#include "halo2/native_tu15_prt_cache_data.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include "halo2/source_model_render_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <bit>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
using Json=nlohmann::json;
unsigned checks{},negatives{};
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
template<class F>void reject(F&& f,const char* why){bool bad{};try{f();}catch(const std::exception&){bad=true;}check(bad,why);++negatives;}
struct Bank{std::uint32_t address;unsigned bytes;};
constexpr std::array banks{Bank{0x4c5160u,108u},Bank{0x4c51d0u,2496u},Bank{0x515ec8u,13u}};
constexpr std::uint32_t stack=0x61008000u,scratch=0x1000000u;
void word(SourceRuntimeMemory& m,std::uint32_t p,std::uint32_t value){std::array<std::byte,4>b{};for(unsigned i=0;i<4;++i)b[i]=std::byte(value>>(8*i));m.write(p,b);}
std::vector<std::byte> hex(std::string_view s){check(!(s.size()%2),"odd original hex bytes");std::vector<std::byte>b;for(unsigned i=0;i<s.size();i+=2)b.push_back(std::byte(std::stoul(std::string(s.substr(i,2)),nullptr,16)));return b;}
std::string hex(std::span<const std::byte> bytes){constexpr char digits[]="0123456789abcdef";std::string s;for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);s+=digits[v>>4];s+=digits[v&15];}return s;}
void inputs(SourceRuntimeMemory& m,std::string_view kind){
 m.mount(0x4ecc14u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"isolated explicit PRT lightmap and BSP bindings");
 m.mount(0x4920c0u,std::array<std::byte,8>{},SourceRuntimeMemory::Access::ReadWrite,"isolated explicit completed double clock");
 m.mount(0x46a0e8u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadOnly,"authenticated original PRT zero-angle operand");
 m.mount(0x46a0d0u,std::array{std::byte{},std::byte{},std::byte{0x80},std::byte{0x3f}},SourceRuntimeMemory::Access::ReadOnly,"authenticated original PRT one operand");
 word(m,0x4920c4u,0x3ff00000u);
 m.mount(stack-4096u,std::array<std::byte,8192>{},SourceRuntimeMemory::Access::ReadWrite,"isolated genuine PRT caller stack");
 m.mount(scratch,std::array<std::byte,65536>{},SourceRuntimeMemory::Access::ReadWrite,"isolated typed PRT lightmap descriptor and coefficients");
 word(m,stack,0x70000000u);word(m,0x4ecc14u,kind=="null-lightmap"?0:scratch);word(m,0x4ecc18u,scratch+0x1000u);
 word(m,0x515eccu,0);word(m,0x515ed0u,0);m.write(0x515ed4u,std::array<std::byte,1>{});
 if(kind=="null-lightmap")return;
 word(m,scratch+0x80u,kind=="empty-lightmap"?0:1);word(m,scratch+0x84u,scratch+0x2000u);
 word(m,scratch+0x201cu,0);word(m,scratch+0x2004u,7);word(m,scratch+0x1008u,7);
 const unsigned count=kind=="two-SH"?2:1;
 word(m,scratch+0x2038u,count);word(m,scratch+0x203cu,scratch+0x3000u);word(m,scratch+0x34u,0x40000000u);
 for(unsigned j=0;j<count;++j)for(unsigned i=0;i<27;++i)word(m,scratch+0x300cu+j*0xdcu+i*4u,std::bit_cast<std::uint32_t>(float(i+1+j*27)/32.f));
}
void portable(){
 SourceRuntimeMemory m;mount_native_tu15_prt_cache_data(m);const auto before=m.checkpoint();check(before.regions.size()==3,"cold PRT owner region count differs");
 for(const auto [p,n]:banks){check(m.view(p,n).size()==n,"cold PRT owner extent differs");check(!m.is_read_only(p,n),"PRT source destination is immutable");check(std::ranges::all_of(m.view(p,n),[](auto b){return b==std::byte{};}),"cold PRT storage was preinitialized with a result");check(m.native_address(p,n).value>UINT32_MAX,"PRT backing narrowed to source32 token");}
 for(auto p:{0x4c515fu,0x4c51ccu,0x4c51cfu,0x4c5b90u,0x515ed5u})reject([&]{m.view(p,1);},"PRT owner admitted alignment gap or neighboring storage");
 word(m,0x4c51d0u,0xabc12345u);const auto live=m.checkpoint();reject([&]{mount_native_tu15_prt_cache_data(m);},"PRT mount reset an existing live owner");check(m.checkpoint()==live,"duplicate cold mount mutated actual runtime values");
 for(const auto [p,n]:banks)for(const auto offset:{0u,n/2u,n-1u}){
  SourceRuntimeMemory custom;custom.mount(p+offset,std::array{std::byte{0xa5}},SourceRuntimeMemory::Access::ReadWrite,"foreign PRT overlap");const auto original=custom.checkpoint();reject([&]{mount_native_tu15_prt_cache_data(custom);},"PRT helper accepted a foreign partial owner");check(custom.checkpoint()==original,"overlap rejection partially published cold banks");
 }
 SourceRuntimeMemory reset;mount_native_tu15_prt_cache_data(reset);inputs(reset,"null-lightmap");SourcePhysicsWorldRuntimeAbi a{};a.gp[4]=stack;a.eflags=0x202u;unsigned calls{},pc{};SourcePhysicsWorldRuntimeServices s;s.instruction=[&](auto p){pc=p;};s.required_owner=[&](auto&,auto&,auto&,auto&){++calls;throw std::runtime_error("unexpected required PRT service");};source_run_model_render_tu15(reset,0x22830u,a,{0x27f,0x1f80},s);check(!calls&&pc==0x22c0au&&a.gp[4]==stack+4,"whole PRT reset failed genuine cold branch");
 for(unsigned row=0;row<24;++row){const auto b=reset.view(0x4c51d0u+row*104u,104u);for(unsigned i=0;i<104;++i){const auto expected=(i<6u||(i>=8u&&i<12u))?std::byte{0xff}:std::byte{};check(b[i]==expected,"PRT source reset did not retain exact24x104 layout");}}
 auto complete=native_tu15_world_memory({0x60212000u,{},"isolated source-free PRT factory tick"});for(const auto [p,n]:banks)check(std::ranges::all_of(complete.view(p,n),[](auto b){return b==std::byte{};}),"normal native factory lacks exact cold PRT owner");
 for(auto p:{0x10000u,0x22830u})reject([&]{complete.view(p,4);},"PRT factory imports an executable/header instead of data");
}
unsigned differential(const Json& proof){
 check(proof.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&proof.at("instruction_patches").empty()&&proof.at("supplied_callee_returns").empty(),"original PRT proof authority differs");check(proof.at("cases").size()==12,"original PRT coverage differs");
 unsigned compared{};
 for(const auto& expected:proof.at("cases")){
  SourceRuntimeMemory m;mount_native_tu15_prt_cache_data(m);const auto fill=std::byte(expected.at("fill").get<unsigned>());for(const auto [p,n]:banks)std::ranges::fill(m.mutable_view(p,n),fill);inputs(m,expected.at("kind").get<std::string>());
  SourcePhysicsWorldRuntimeAbi a{};a.gp=expected.at("initial_gp").get<decltype(a.gp)>();a.eflags=expected.at("initial_eflags");unsigned count{};Json stores=Json::array();SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++count;};services.required_owner=[](auto&,auto&,auto&,auto&){throw std::runtime_error("original PRT fixture requested a supplied callee result");};
  source_run_model_render_tu15(m,0x22830u,a,{0x27f,0x1f80},services,[&](const auto& store){if(std::ranges::any_of(banks,[&](auto b){return store.address>=b.address&&std::uint64_t(store.address)+store.written.size()<=std::uint64_t(b.address)+b.bytes;}))stores.push_back({{"pc",store.instruction},{"address",store.address},{"before",hex(store.before)},{"after",hex(store.written)}});});
  check(a.gp==expected.at("returned_gp").get<decltype(a.gp)>(),"PRT returned GP differs from untouched CPU");check((a.eflags&0xcc5u)==(expected.at("returned_eflags").get<unsigned>()&0xcc5u),"PRT flags differ from untouched CPU");check(count==expected.at("instructions").get<unsigned>(),"PRT executed instruction count differs");check(stores==expected.at("stores"),"PRT exact ordered source stores differ from original");
  for(const auto& image:expected.at("postimages")){const auto b=hex(image.at("bytes").get<std::string>());check(std::ranges::equal(m.view(image.at("address").get<unsigned>(),b.size()),b),"PRT complete source-bank postimage differs from original");compared+=b.size();}
 }
 return compared;
}
}
int main(int argc,char**argv)try{check(argc==1||argc==2,"usage: PRT-test [original-proof.json]");portable();unsigned original_bytes{};if(argc==2){if(!*argv[1]||!std::filesystem::exists(argv[1]))return 77;std::ifstream in(argv[1]);original_bytes=differential(Json::parse(in));}std::cout<<"PASS "<<checks<<" checks / "<<negatives<<" negatives / "<<original_bytes<<" original bank bytes; exact PRT cold ownership and genuine reset\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
