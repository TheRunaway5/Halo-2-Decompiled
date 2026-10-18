#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
constexpr unsigned device=0x40f0c0,scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000;
Json state(const SourcePhysicsWorldRuntimeAbi&a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
void put(SourceRuntimeMemory&m,unsigned p,unsigned v){m.write(p,std::as_bytes(std::span(&v,1)));}
unsigned word(const SourceRuntimeMemory&m,unsigned p){return Bytes(m.view(p,4)).u32(0);}
void inputs(SourceRuntimeMemory&m) {
 mount_source_particle_graphics_layout_data_tu15(m);
 for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,20>{{
  {device+8,0x2498},{0x411568,4},{0x4116c0,4},{0x4117b4,4},{0x4918e4,12},{0x4920d8,4},
  {0x492294,1},{0x4922a0,4},{0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4},
  {0x47362c,24},{0x4c61d0,76},{0x4c685c,4},{0x4f63b8,4},{0x4f54f0,8},{0x4c66f8,4},
  {0x56dfa0,16},{scratch,65536}}})
  m.mount(p,std::vector<std::byte>(n,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
   "declared CPU-only pass/gate/device/stack inputs; no original startup, queue retirement or native GPU history");
 put(m,0x411568,device);put(m,device+8,0x200);put(m,0x4117b4,2);put(m,device+0x794,scratch+0x400);
 for(unsigned p:{device+0x95c,device+0x960})put(m,p,0x3f800000);
 put(m,device+0x944,0x3a800000);put(m,device+0x948,0x4b7fffff);put(m,device+0x93c,0x3c800000);
 const std::array<unsigned,6> viewport{0,0,640,480,0,0x3f800000};m.write(device+0xee0,std::as_bytes(std::span(viewport)));
 put(m,device+0xef8,0x3f000000);put(m,device+0xefc,0x3f000000);put(m,device+0x1c18,0);
 put(m,0x4922f4,0x3c800000);put(m,0x4922f8,0x44800000);put(m,0x4c2aa0,17);put(m,0x4c2aa4,19);
 put(m,0x4f63b8,scratch+0x2000);put(m,scratch+0x2c14,2);put(m,0x56dfa8,1);put(m,0x4c66f8,0xffffffffu);
 put(m,0x4f54f0,scratch+0x3000);put(m,0x4f54f4,scratch+0x4000);put(m,scratch+0x4044,scratch+0x5000);
 put(m,scratch+0x300c,0xffffffffu);put(m,scratch+0x5000+2*0x21c+0x2c,0xffffffffu);
 put(m,0x47362c,0);put(m,0x473630,0);put(m,0x4c6218,0);put(m,0x4922a0,0);
}
void bank(const std::filesystem::path&p,const SourceRuntimeMemory&m,const SourceParticleConstantStorageTu15&s){std::ofstream f(p,std::ios::binary);const auto emit=[&](unsigned a,std::span<const std::byte>b){unsigned n=b.size();f.write(reinterpret_cast<const char*>(&a),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);};m.visit_committed(emit);s.visit_committed(emit);if(!f)throw FormatError("Cannot export pass-gate evidence");}
}
int main(int argc,char**argv)try {
 if(argc>2)throw FormatError("Optional new original pass-gate output directory");const std::filesystem::path out=argc==2?argv[1]:"";
 if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve prior pass-gate evidence");std::filesystem::create_directories(out);}
 unsigned count=0;
 for(unsigned row=0;row<17;++row) {
  SourceRuntimeMemory memory;inputs(memory);const bool full=row>=9;const unsigned branch=full?(row-9)%4:0;
  bool expected=true;
  if(!full) {
   expected=row==1 || row==3 || row==4 || row==8;
   if(row==0)put(memory,0x56dfa8,0);
   if(row>=2)put(memory,0x4c66f8,0);
   const unsigned fraction=row==2?0x3f000000:row==6?0x7fc12345:row==7?0xbf800000:row==8?0x40000000:0x3f800000;
   put(memory,scratch+0x20e4,fraction);
   if(row==4 || row==5 || row==8)put(memory,scratch+0x300c,0x12340002);
   if(row==5)put(memory,scratch+0x5000+2*0x21c+0x2c,22);
  }else {
   if(branch==1)put(memory,0x473630,scratch+0x2000);
   if(branch==2)put(memory,0x4c6218,0x3f800000);
   if(branch==3)put(memory,0x47362c,0x100); // Nonzero original47362D flag with an explicitly null4922A0 input.
  }
  const unsigned serial=full && row>=13?0xffffffffu:17;put(memory,0x4c685c,serial);
  SourceParticleConstantStorageTu15 storage;std::array<ShaderVector,192> inherited{};storage.begin(memory,inherited);
  SourcePhysicsWorldRuntimeAbi header{};header.gp[0]=scratch+0x400;header.gp[1]=4;header.gp[4]=sp;header.eflags=0x202;
  header.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(memory,sp,stop);source_run_effect_particle_draw_tu15(storage,0x400a60,header,*header.floating_point,{});
  const std::array<unsigned,7> args{stop,1,1,0x11223344,0x55667788,0,0};memory.write(sp,std::as_bytes(std::span(args)));
  SourcePhysicsWorldRuntimeAbi a{};a.gp={0x12345678,0xabcdef01,0x89abcdef,0x76543210,sp,0x24681357,0x13572468,0xfedcba98};
  a.xmm[0]={0x80000000,0x89abcdef,0x76543210,0xfedcba98};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=a;
  const unsigned entry=full?0x2bcb0:0x15c030;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](auto pc){trace.push_back(pc);};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("Pass-gate case reached an unowned original service");};
  const auto path=out/std::to_string(count++);if(!out.empty()){std::filesystem::create_directory(path);bank(path/"before.bin",memory,storage);}
  source_run_effect_particle_draw_tu15(storage,entry,a,*a.floating_point,services);
  if(a.gp[4]!=sp+(full?28:4) || a.gp[3]!=before.gp[3] || a.gp[5]!=before.gp[5] || a.gp[6]!=before.gp[6] || a.gp[7]!=before.gp[7] || a.x87_depth || (a.gp[0]&255)!=unsigned(expected))
   throw FormatError("Original pass gate or complete skipped-pass ABI differs");
  if(full) {
   if(word(memory,0x4c685c)!=serial+1u || word(memory,0x4918e4)!=serial+1u || word(memory,0x4918e8)!=0 || word(memory,0x4918ec)!=0x80000000u || word(memory,0x4c2aa0) || word(memory,0x4c2aa4) || memory.view(0x4c2ab0,1)[0]!=std::byte{0})
    throw FormatError("Complete skipped pass omitted its original packet/reset/finish state");
   if(std::find(trace.begin(),trace.end(),0x40e80)!=trace.end())throw FormatError("Skipped pass executed queue construction");
   const bool second_gate=std::find(trace.begin(),trace.end(),0x15c030)!=trace.end();if(second_gate!=(branch!=3))throw FormatError("Original first gate did not own pass selection");
  }else if(word(memory,0x4c2aa0)!=17 || word(memory,0x4c2aa4)!=19 || word(memory,0x4c685c)!=serial)
   throw FormatError("Selection predicate modified pass serial or queues");
  if(!out.empty()){bank(path/"after.bin",memory,storage);std::ofstream(path/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(a)},{"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"row",row},{"branch",branch},{"complete_skipped_pass",full},{"queue_retirement_history_claimed",false},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()},{"full_goal_complete",false}}.dump(2)<<'\n';}
 }
 std::cout<<"PASS9 original view predicates and8 complete skipped passes: actual initializer, selection, counter/wrap and finish owners; no queue builder or supplied callee return\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
