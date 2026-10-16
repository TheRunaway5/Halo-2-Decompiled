#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cmath>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
Json state(const SourcePhysicsWorldRuntimeAbi &a) {
 return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},
  {"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};
}
void export_bank(const std::filesystem::path &path,const SourceRuntimeMemory &memory,const SourceParticleConstantStorageTu15 &storage) {
 std::ofstream out(path,std::ios::binary);const auto emit=[&](unsigned p,std::span<const std::byte>b){const unsigned n=b.size();out.write(reinterpret_cast<const char*>(&p),4);out.write(reinterpret_cast<const char*>(&n),4);out.write(reinterpret_cast<const char*>(b.data()),n);};memory.visit_committed(emit);storage.visit_committed(emit);if(!out)throw FormatError("Cannot export source viewport sequence");
}
}
int main(int argc,char **argv)try {
 if(argc>2)throw FormatError("Optional new viewport-state output directory");const std::filesystem::path out=argc==2?argv[1]:"";
 if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier viewport-state evidence");std::filesystem::create_directories(out);}
 constexpr unsigned device=0x40f0c0,scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000;unsigned cases=0;
 const auto layout=source_particle_d24s8_layout_tu15(640,480);
 for(unsigned row=0;row<8;++row) {
  SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
  for(const auto [p,n]:std::array<std::pair<unsigned,unsigned>,13>{{
   {device+8,0x2498},{0x411568,4},{0x411570,4},{0x4116c0,4},{0x4117b4,4},{0x4117d8,4},
   {0x4918e4,12},{0x492294,1},{0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4},{0x4116ec,4}}})
   memory.mount(p,std::vector<std::byte>(n,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,
    "declared CPU viewport/target/current-FVF inputs; no GPU or original boot history");
  memory.mount(scratch,std::vector<std::byte>(65536,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared CPU viewport sequence stack and inputs");
  const auto put=[&](unsigned p,unsigned v){memory.write(p,std::as_bytes(std::span(&v,1)));};
  const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};
  put(0x411568,device);put(0x411570,0);put(0x4117b4,row==6?0:2);put(0x4117d8,row==5?1:0);
  put(0x4116ec,(row==5 || row==7)?1:0); // Declared CPU pretransformed fog-program selection.
  put(device+8,row==5?0x8200:0x200);put(device+0x794,scratch+0x400);
  for(unsigned p:{device+0x95c,device+0x960})put(p,0x3f800000);
  put(device+0x954,640);put(device+0x958,480);put(device+0x944,0x3a800000);put(device+0x948,layout.depth_scale_bits);put(device+0x93c,0x3c800000);
  put(device+0xef8,0x3f000000);put(device+0xefc,0x3f000000);
  put(device+0x1c18,row==4?3:2);put(device+0x1c08,0);put(device+0x1c0c,layout.depth_scale_bits);
  put(device+0x1c10,0);put(device+0x1c14,layout.depth_scale_bits);
  for(unsigned p:{device+0x1a04,device+0x1a14,device+0x1a18})put(p,scratch+0x300);
  // Only produced extent metadata is read by4021E0. No Common/Data/refcount
  // or physical-address fields are supplied as a supposed SDK allocation.
  const std::array<unsigned,2> extent{layout.format_word,layout.size_word};memory.write(scratch+0x30c,std::as_bytes(std::span(extent)));
  //407160's first stack argument is its output.400920 therefore writes980
  // and reads CA0. Whole4020D0(index1) below owns that projection input and
  // its93C/940/944 derivatives; leave the output poisoned until produced.
  std::array<float,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  if(row==1 || row==5) {
   const float near=row==1?.015625f:.25f,far=row==1?1024.f:64.f;
   matrix[0]=1.25f;matrix[5]=1.75f;matrix[10]=far/(near-far);matrix[11]=-1;
   matrix[14]=(far*near)/(near-far);matrix[15]=0;
  }
  if(row==2){matrix[8]=.125f;matrix[9]=-.25f;matrix[10]=-.125f;matrix[14]=.5f;}
  if(row==3){matrix[10]=-1;matrix[11]=-1;matrix[15]=0;}
  if(row==6){matrix[2]=.125f;matrix[6]=-.0625f;matrix[10]=-1.25f;matrix[11]=-1;matrix[14]=-.25f;matrix[15]=0;}
  // The complete FVF header at+400 occupies280 bytes, through+518.
  // Keep the projection input outside that real producer's output extent.
  memory.write(scratch+0x1000,std::as_bytes(std::span(matrix)));
  std::array<unsigned,6> viewport{0,0,640,480,0,0x3f800000};
  if(row==1 || row==5)viewport={320,240,320,240,0,0x3f800000};
  if(row==2)viewport={32,16,2000,1000,0x3e4ccccd,0x3f666666};
  if(row==3)viewport={0,0,1,1,0,0x3f800000};
  if(row==4)viewport={20,30,600,400,0x3e800000,0x3f400000};
  memory.write(scratch+0x100,std::as_bytes(std::span(viewport)));
  const std::array<unsigned,6> retained_viewport{4,6,632,468,0,0x3f800000};memory.write(device+0xee0,std::as_bytes(std::span(retained_viewport)));
  put(0x4922f4,0x3c800000);put(0x4922f8,0x44800000);put(0x4c2aa0,17);put(0x4c2aa4,19);
  const std::array<unsigned,3> frame{101+row,0,0};memory.write(scratch+0x80,std::as_bytes(std::span(frame)));
  SourceParticleConstantStorageTu15 storage;std::array<ShaderVector,192> inherited{};storage.begin(memory,inherited);
  SourcePhysicsWorldRuntimeAbi header{};header.gp[0]=scratch+0x400;header.gp[1]=(row%2)?4:2;header.gp[4]=sp;header.eflags=0x202;header.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(sp,stop);
  source_run_effect_particle_draw_tu15(storage,0x400a60,header,*header.floating_point,{});
  for(unsigned phase=0;phase<4;++phase) {
   const unsigned entry=phase==0?0x4020d0:phase==1?0x4021e0:phase==2?0x13430:0x15700;
   const std::vector<unsigned> args=phase==1?std::vector<unsigned>{stop,row==7?0:scratch+0x100}:phase==3?std::vector<unsigned>{stop,0,0}:std::vector<unsigned>{stop};
   memory.write(sp,std::as_bytes(std::span(args)));SourcePhysicsWorldRuntimeAbi a{};
   a.gp={0x12345678,scratch+0x80,0xabcdef01,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};
   if(phase==0){a.gp[0]=1;a.gp[2]=scratch+0x1000;}
   a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=a;
   std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Viewport sequence requires an unowned source/SDK service");};
   const auto path=out/std::to_string(cases++);if(!out.empty()){std::filesystem::create_directory(path);export_bank(path/"before.bin",memory,storage);}
   source_run_effect_particle_draw_tu15(storage,entry,a,*a.floating_point,services);
   if(a.gp[4]!=sp+args.size()*4 || a.gp[3]!=before.gp[3] || a.gp[5]!=before.gp[5] || a.gp[6]!=before.gp[6] || a.gp[7]!=before.gp[7] || a.x87_depth)
    throw FormatError("Original viewport sequence ABI differs");
   if(phase==0) {
    if(!std::equal(memory.view(device+0xca0,64).begin(),memory.view(device+0xca0,64).end(),std::as_bytes(std::span(matrix)).begin()))throw FormatError("Original projection setter did not own CA0");
    bool replaced=false;for(unsigned n=0;n<16;++n){const auto bits=word(device+0x980+n*4);replaced|=bits!=0xa5a5a5a5u;if(!std::isfinite(std::bit_cast<float>(bits)))throw FormatError("Original viewport multiply produced a nonfinite matrix");}
    if(!replaced)throw FormatError("Original viewport multiply retained poisoned output");
    if(word(0x4c2aa0)!=17 || word(0x4c2aa4)!=19)throw FormatError("Projection setter reset particle queues");
   }else if(phase==1) {
    if(word(0x4c2aa0)!=17 || word(0x4c2aa4)!=19 || !(word(0x411570)&0x200))throw FormatError("Viewport setter altered queues or omitted matrix dirtiness");
    if(row==2 && (word(device+0xee8)!=608 || word(device+0xeec)!=464))throw FormatError("Original viewport did not clamp to its produced surface extent");
    if(row==7 && !std::equal(memory.view(device+0xee0,24).begin(),memory.view(device+0xee0,24).end(),std::as_bytes(std::span(retained_viewport)).begin()))throw FormatError("Null viewport replaced its retained rectangle");
   }else if(word(0x4c2aa0) || word(0x4c2aa4))throw FormatError("Original per-view initialization failed to own queue reset");
   if(!out.empty()){export_bank(path/"after.bin",memory,storage);std::ofstream(path/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(a)},{"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"row",row},{"phase",phase},{"command_bytes",storage.commands().size()},{"original_boot_or_gpu_history_claimed",false},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';}
  }
 }
 std::cout<<"PASS32 complete original projection/viewport/reset/restore calls: actual CA0 input and980 output, perspective/default/oblique-like matrices, split/clamp/null/W modes and retained queue/ABI contracts\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
