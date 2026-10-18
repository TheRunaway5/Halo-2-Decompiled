#include "halo2/source_particle_view_pass_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
struct ObservationStop{};
Json state(const SourcePhysicsWorldRuntimeAbi&a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
void bank(const std::filesystem::path&p,const SourceRuntimeMemory&m){std::ofstream f(p,std::ios::binary);m.visit_committed([&](unsigned a,std::span<const std::byte>b){const unsigned n=b.size();f.write(reinterpret_cast<const char*>(&a),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);});if(!f)throw FormatError("Cannot export original pass input evidence");}
}
int main(int argc,char**argv)try {
 if(argc>2)throw FormatError("Optional new pass-packet evidence directory");const std::filesystem::path out=argc==2?argv[1]:"";
 if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier pass-packet evidence");std::filesystem::create_directories(out);}
 constexpr unsigned counter=0x4c685c,scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000;unsigned cases=0;
 for(const unsigned serial:{0u,17u,0x7fffffffu,0xfffffffeu,0xffffffffu})
 for(const auto[mode,distance]:std::array<std::pair<unsigned,unsigned>,6>{{{0,0},{1,0x3e000000},{2,0x42000000},{0xffffffffu,0x80000000},{0,0x7fc12345},{1,0x7f812345}}}) {
  SourceRuntimeMemory values;values.mount(counter,std::as_bytes(std::span(&serial,1)),SourceRuntimeMemory::Access::ReadWrite,"actual source pass counter value boundary");
  const auto packet=source_particle_view_pass_tu15(values,mode,distance);
  if(packet.words!=std::array<unsigned,3>{serial+1u,mode,distance} || Bytes(values.view(counter,4)).u32(0)!=packet.words[0])throw FormatError("Pass value boundary did not retain all three original words");
  SourceRuntimeMemory memory;memory.mount(counter,std::as_bytes(std::span(&serial,1)),SourceRuntimeMemory::Access::ReadWrite,"declared original pass counter input");
  memory.mount(scratch,std::vector<std::byte>(65536,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared ordinary13660 caller stack; no GPU/callee return is supplied");
  const std::array<unsigned,7> args{stop,1,1,0x11223344,0x55667788,0,mode};memory.write(sp,std::as_bytes(std::span(args)));
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp={0x12345678,0xabcdef01,0x89abcdef,0x76543210,sp,0x24681357,0x13572468,0xfedcba98};
  abi.xmm[0]={distance,0x89abcdef,0x76543210,0xfedcba98};abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=abi;
  const auto path=out/std::to_string(cases++);if(!out.empty()){std::filesystem::create_directory(path);bank(path/"before.bin",memory);}
  std::vector<unsigned> trace;bool stopped=false;SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](unsigned pc){if(pc==0x13430)throw ObservationStop{};trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("Pass packet reached an unexpected missing service before its observation boundary");};
  try{source_run_effect_particle_draw_tu15(memory,0x2bcb0,abi,*abi.floating_point,services);}catch(const ObservationStop&){stopped=true;}
  if(!stopped || trace.empty() || trace.back()!=0x2bcea || abi.gp[1]!=sp-12 || abi.gp[4]!=sp-32 ||
     Bytes(memory.view(abi.gp[1],12)).u32(0)!=packet.words[0] || Bytes(memory.view(abi.gp[1],12)).u32(4)!=packet.words[1] ||
     Bytes(memory.view(abi.gp[1],12)).u32(8)!=packet.words[2] || Bytes(memory.view(counter,4)).u32(0)!=packet.words[0])
   throw FormatError("Whole original pass entry did not produce the actual13430 packet/counter");
  if(!out.empty()){bank(path/"after.bin",memory);std::ofstream(path/"case.json")<<Json{{"entry",0x2bcb0},{"before",state(before)},{"after",state(abi)},
    {"instructions",trace},{"return_address",0x13430},{"completed",false},{"missing_owner",0},{"observation_boundary",0x13430},
    {"callee_executed",false},{"original_returns_supplied",Json::array()},{"value_packet_words",packet.words},{"initial_serial",serial},
    {"whole_pass_complete",false},{"full_goal_complete",false}}.dump(2)<<'\n';}
 }
 std::cout<<"PASS30 source pass serial/packet values and whole-entry prefixes: ordinary caller arguments, wrap/sign boundaries, raw NaN/negative-zero payloads; no13430 execution or supplied return\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
