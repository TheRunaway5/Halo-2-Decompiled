// Test-only replay of an actual live14FEC0 input against untouched i386 output.
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/physics_phantom_world_tu15.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <cstring>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
std::vector<std::byte> bytes(const std::filesystem::path &p){std::ifstream f(p,std::ios::binary|std::ios::ate);check(bool(f),"Replay input absent");std::vector<std::byte> b(static_cast<std::size_t>(f.tellg()));f.seekg(0);f.read(reinterpret_cast<char*>(b.data()),b.size());check(bool(f),"Replay input truncated");return b;}
std::string hex(std::span<const std::byte> b){constexpr char digits[]="0123456789abcdef";std::string s;for(auto v:b){auto n=std::to_integer<unsigned>(v);s+=digits[n>>4];s+=digits[n&15];}return s;}
}
int main(int argc,char **argv)try {
  check(argc==4,"Supply live player capture, original hardware output directory and report path");
  const std::filesystem::path input=argv[1],original=argv[2];
  if(!std::filesystem::exists(input/"capture.json") || !std::filesystem::exists(original/"output.bin"))return 77;
  Json captured,reference,trace;std::ifstream(input/"capture.json")>>captured;std::ifstream(original/"report.json")>>reference;std::ifstream(original/"trace.json")>>trace;
  check(captured.at("entry")==0x14fec0u && reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Player replay requires English TU1.5");
  SourceRuntimeMemory memory;for(const auto &r:captured.at("regions"))memory.mount(r.at("address"),bytes(input/r.at("before_file").get<std::string>()),SourceRuntimeMemory::Access::ReadWrite,"Actual captured player scheduler input");
  const auto &before=captured.at("before");SourcePhysicsWorldRuntimeAbi abi{};
  abi.gp=before.at("gp").get<decltype(abi.gp)>();abi.xmm=before.at("xmm").get<decltype(abi.xmm)>();abi.eflags=before.at("eflags");abi.fs_base=before.at("fs_base");abi.x87_status=before.at("x87_status");
  abi.x87_depth=before.at("x87_values").size();for(unsigned n=0;n<abi.x87_depth;++n){const auto s=before.at("x87_values").at(n).get<std::string>();for(unsigned i=0;i<10;++i)reinterpret_cast<unsigned char*>(&abi.x87[n])[i]=std::stoul(s.substr(i*2,2),nullptr,16);}
  SourcePhysicsFactoryFloatingPoint fp{before.at("x87_control_word"),before.at("mxcsr")};abi.floating_point=fp;
  std::vector<std::uint32_t> executed;SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](auto pc){executed.push_back(pc);};
  services.required_owner=[&](auto &m,const auto &call,auto &a,const auto &observe){
    auto state=a.floating_point.value_or(fp);
    if(source_physics_live_support_tu15_owns_entry(call.target))source_run_physics_live_support_tu15(m,call.target,a,state,services,observe);
    else if(call.target==0x30bd0u) {source_run_physics_phantom_world_tu15_runtime(m,call.target,a,state,services,observe);a.floating_point=state;}
    else if(call.target==0xe65a0u)source_run_damage_delivery_runtime(m,0xe5680u,a,SourcePhysicsContactImage::TitleUpdate15,state,services,observe);
    else if(call.target==0x122b60u)source_run_physics_world_runtime(m,0x1218c0u,a,SourcePhysicsContactImage::TitleUpdate15,state,services,observe);
    else throw std::runtime_error("Unbound actual player replay owner "+std::to_string(call.target));
  };
  source_run_physics_live_support_tu15(memory,0x14fec0u,abi,fp,services);
  Json actual{{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},{"x87_status",abi.x87_status},{"x87_control_word",abi.floating_point->x87_control_word},{"mxcsr",abi.floating_point->mxcsr},{"x87_values",Json::array()}};
  for(unsigned n=0;n<abi.x87_depth;++n)actual["x87_values"].push_back(hex(std::as_bytes(std::span(&abi.x87[n],1)).first(10)));
  auto expected=reference.at("original_abi");Json differences=Json::array();
  for(const auto &[key,value]:actual.items()) {
    if(key=="eflags") {if((value.get<unsigned>()&0xfffffdffu)==(expected.at(key).get<unsigned>()&0xfffffdffu))continue;}
    else if(value==expected.at(key))continue;
    differences.push_back({{"field",key},{"native",value},{"original",expected.at(key)}});
  }
  auto regions=captured.at("regions");std::sort(regions.begin(),regions.end(),[](const auto &a,const auto &b){return a.at("address").template get<unsigned>()<b.at("address").template get<unsigned>();});
  std::ifstream output(original/"output.bin",std::ios::binary);output.seekg(276);std::size_t compared{};unsigned memory_differences{};
  for(const auto &r:regions){const auto size=r.at("size").get<std::size_t>();std::vector<std::byte> expected_bytes(size);output.read(reinterpret_cast<char*>(expected_bytes.data()),size);check(bool(output),"Original player output truncated");const auto actual_bytes=memory.view(r.at("address").get<unsigned>(),size);memory_differences+=!std::ranges::equal(expected_bytes,actual_bytes);compared+=size;}
  const auto original_path=trace.at("instructions").get<decltype(executed)>();
  const bool exact_path=executed==original_path;
  // These four original REP sites have one extra native entry notification
  // before their per-iteration callbacks. Keep raw paths and require precisely
  // that difference; original CPU execution and output are never modified.
  std::size_t ni{},oi{};unsigned rep_entry_callbacks{};bool matched_path=true;
  while(ni<executed.size() && oi<original_path.size()) {
    const auto pc=executed[ni];if(pc!=original_path[oi]){matched_path=false;break;}
    auto ne=ni+1,oe=oi+1;while(ne<executed.size() && executed[ne]==pc)++ne;while(oe<original_path.size() && original_path[oe]==pc)++oe;
    if(ne-ni!=oe-oi) {
      const bool rep=pc==0x150299u || pc==0xc93dcu || pc==0x1531abu || pc==0xc93bdu;
      const auto code=memory.view(pc,2);
      check(!rep || (code[0]==std::byte{0xf3} && code[1]==(pc==0xc93dcu?std::byte{0xab}:std::byte{0xa5})),"REP diagnostic site changed original bytes");
      if(!rep || ne-ni!=oe-oi+1){matched_path=false;break;}++rep_entry_callbacks;
    }
    ni=ne;oi=oe;
  }
  matched_path &= ni==executed.size() && oi==original_path.size();
  std::ofstream(std::string(argv[3])+".native-trace.json")<<Json(executed).dump()<<'\n';
  Json report{{"reference","English TU1.5 only"},{"passed",differences.empty() && !memory_differences && matched_path},{"differences",differences},{"memory_region_differences",memory_differences},{"compared_memory_bytes",compared},{"instructions",executed.size()},{"exact_instruction_path",exact_path},{"instruction_path_with_explicit_rep_notifications",matched_path},{"extra_native_rep_entry_notifications",rep_entry_callbacks},{"original_instructions",original_path.size()},{"native_abi",actual},{"original_abi",expected},{"eflags_comparison_mask",0xfffffdffu},{"boundary","Actual captured complete player scheduler, unchanged original hardware output and all retained bytes; only observed original callees are bound, none are replaced"}};
  std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<"player scheduler passed="<<report.at("passed")<<" instructions="<<executed.size()<<" memory="<<compared<<" differences="<<differences.dump()<<'\n';return report.at("passed").get<bool>()?0:2;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
