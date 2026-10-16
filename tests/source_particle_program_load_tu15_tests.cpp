#define SDL_MAIN_HANDLED
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
template<class F>void rejected(F &&f,const char *why){bool stopped=false;try{f();}catch(const std::exception &){stopped=true;}if(!stopped)throw FormatError(why);}
Json state(const SourcePhysicsWorldRuntimeAbi &a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
void emit(std::ofstream &f,unsigned p,std::span<const std::byte>b){const unsigned n=b.size();f.write(reinterpret_cast<const char*>(&p),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);}
void record(const std::filesystem::path &out,unsigned row,unsigned entry,const SourcePhysicsWorldRuntimeAbi &before,const SourcePhysicsWorldRuntimeAbi &after,const std::vector<unsigned>&trace,unsigned mask){if(out.empty())return;std::ofstream(out/std::to_string(row)/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(after)},{"instructions",trace},{"return_address",0x60300000},{"completed",true},{"missing_owner",0},{"defined_flags_mask",mask},{"native_program_interface_declared",true},{"original_sdk_boot_or_current_shader_history_claimed",false},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';}
}
int main(int argc,char **argv)try {
 if(argc<4 || argc>6)throw FormatError("Supply shaders, Backwash map, shared maps, optional new output directory and captured resource fragments");
 if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
 const std::filesystem::path out=argc>=5?argv[4]:"";if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier program load evidence");std::filesystem::create_directories(out);}
 Cache cache(argv[2],argv[3]);const auto render=cache.load_particle_system_render_definition(0xe76105ec,0,0);
 auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(render.particle_datum,15,0,0));
 constexpr unsigned scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000;
 SourceRuntimeMemory memory;memory.mount(scratch,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"declared native program caller stack/code/cache");
 const auto put=[&](unsigned p,unsigned v){memory.write(p,std::as_bytes(std::span(&v,1)));};
 const auto abi=[&]{SourcePhysicsWorldRuntimeAbi a{};a.gp={0x12345678,0x87654321,0xabcdef01,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;};
 memory.write(scratch+0x100,material->vertex.program);
 std::array<ShaderVector,192> inherited{};for(unsigned n=0;n<192;++n)for(unsigned k=0;k<4;++k)inherited[n][k]=std::bit_cast<float>(0x3e000000u+n*4+k);
 std::array<ShaderVector,2> values{{{1,2,3,4},{-1,-2,-3,-4}}};memory.write(scratch+0x800,std::as_bytes(std::span(values)));
 SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());auto *window=SDL_CreateWindow("Original particle program packets",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);if(!window)throw FormatError(SDL_GetError());
 {
 Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);
 Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);SourceParticleGraphicsBindingTu15 binding(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());binding.begin(memory,inherited);
 put(sp,stop);auto initialize=abi();initialize.gp[0]=0x11;binding.configure(initialize);
 auto upload=[&](unsigned first,unsigned input){put(sp,stop);auto a=abi();a.gp[1]=first;a.gp[2]=input;binding.run(0x4012b0,a);};
 upload(17,scratch+0x800);
 const std::array<unsigned,3> args{stop,scratch+0x100,0};memory.write(sp,std::as_bytes(std::span(args)));
 auto a=abi();auto invalid=a;
 rejected([&]{binding.load_program({},material,invalid);},"Unissued program loader was accepted");
 put(sp+8,1);rejected([&]{binding.load_program(renderer.particle_program_control(),material,invalid);},"A nonzero original program slot was accepted");put(sp+8,0);
 const auto original=material->vertex.program.at(4);memory.write(scratch+0x104,std::array<std::byte,1>{original^std::byte{1}});
 rejected([&]{binding.load_program(renderer.particle_program_control(),material,invalid);},"Foreign source program bytes were accepted");memory.write(scratch+0x104,std::array<std::byte,1>{original});
 const auto prefix=binding.commands().size();
 const auto export_bank=[&](const char *name){if(out.empty())return;std::ofstream f(out/"0"/name,std::ios::binary);memory.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});binding.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});};
 if(!out.empty())std::filesystem::create_directory(out/"0");export_bank("before.bin");const auto before=a;
 // RET has no memory writes. Retain the source-memory result immediately
 // before it, ahead of the subsequent native pipeline/header publication.
 bool retained_source_result=false;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);if(p==0x400dcd){export_bank("after.bin");retained_source_result=true;}};services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Original program loader reached an unowned SDK service");};
 binding.load_program(renderer.particle_program_control(),material,a,services);if(!retained_source_result)throw FormatError("Original loader source result was not retained");record(out,0,0x400d40,before,a,trace,0x8d5);
 if(a.gp[4]!=sp+12 || a.gp[3]!=before.gp[3] || a.gp[5]!=before.gp[5] || a.gp[6]!=before.gp[6] || a.gp[7]!=before.gp[7])throw FormatError("Whole original program loader ABI differs");
 const auto program_end=binding.commands().size();const auto configuration=binding.program_configuration();if(configuration.classification!=7 || configuration.instructions!=51 || program_end-prefix!=852)throw FormatError("Actual source program load lost instructions or packet chunking");
 upload(191,scratch+0x810);auto expected=inherited;expected[17]=values[0];expected[191]=values[1];std::array<ShaderVector,192> decoded{};binding.consume_constants(decoded);
 if(std::memcmp(decoded.data(),expected.data(),sizeof(expected)))throw FormatError("Mixed original program/constants stream changed constant bits");
 const auto malformed=[&](unsigned address,unsigned value){std::array<std::byte,4> saved;binding.read(address,saved);binding.write(address,std::as_bytes(std::span(&value,1)));auto unchanged=inherited;rejected([&]{binding.consume_constants(unchanged);},"Malformed program stream was accepted");if(std::memcmp(unchanged.data(),inherited.data(),sizeof(inherited)))throw FormatError("Malformed program stream partially published constants");binding.write(address,saved);};
 const auto packet=0xf7700000u+unsigned(prefix);
 malformed(packet,0x41ea0);malformed(packet+4,1);malformed(packet+8,0x840b00);malformed(packet+12,Bytes(binding.commands()).u32(prefix+12)^0x80000000u);malformed(packet+8,0x200b00);malformed(0x40f0c0,0xf7700000u+unsigned(program_end-4));
 std::array<std::byte,4> unavailable{};rejected([&]{binding.read(0x410168,unavailable);},"An unowned CPU cache reader was accepted");rejected([&]{binding.write(0x410168,unavailable);},"An unowned CPU cache writer was accepted");
 rejected([&]{binding.load_program(renderer.particle_program_control(),material,a);},"Repeated source program loading was accepted");
 binding.begin(memory,inherited);binding.consume_constants(decoded);if(std::memcmp(decoded.data(),inherited.data(),sizeof(inherited)))throw FormatError("A new command batch retained an old program packet extent");
 // Valid instruction syntax alone cannot authorize an uncompiled program.
 // A failed native realization leaves its pending packets unavailable.
 auto changed=std::make_shared<ParticleMaterial>(*material);changed->vertex.program.at(4)^=std::byte{1};memory.write(scratch+0x100,changed->vertex.program);
 SourceParticleGraphicsBindingTu15 failed(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());failed.begin(memory,inherited);put(sp,stop);initialize=abi();initialize.gp[0]=0x11;failed.configure(initialize);memory.write(sp,std::as_bytes(std::span(args)));auto failed_call=abi();
 rejected([&]{failed.load_program(renderer.particle_program_control(),changed,failed_call);},"An uncompiled original program became resident");
 auto unchanged=inherited;rejected([&]{failed.consume_constants(unchanged);},"A failed native program authorized its pending packets");if(std::memcmp(unchanged.data(),inherited.data(),sizeof(inherited)))throw FormatError("A failed program partially published constants");memory.write(scratch+0x100,material->vertex.program);
 if(!out.empty())std::ofstream(out/"program.json")<<Json{{"class",configuration.classification},{"instructions",configuration.instructions},{"program_packet_bytes",program_end-prefix},{"pipeline",configuration.pipeline},{"vertex_module",configuration.vertex_module},{"program_hash",configuration.program_hash},{"native_program_packet_consumer",true},{"atomic_malformed_cases",6},{"mixed_constant_stream_bits_exact",true},{"gpu_draw_claimed",false},{"full_goal_complete",false}}.dump(2)<<'\n';
 }
 SDL_DestroyWindow(window);SDL_Quit();
 if(argc==6){
  // Exact fragments from a real source-created world. Only the native cache
  // and caller/device inputs below are declared CPU fixture state.
  std::ifstream input(argv[5],std::ios::binary);while(input.peek()!=EOF){unsigned p,n;input.read(reinterpret_cast<char*>(&p),4);input.read(reinterpret_cast<char*>(&n),4);std::vector<std::byte> b(n);input.read(reinterpret_cast<char*>(b.data()),n);if(!input)throw FormatError("Truncated captured program resources");memory.mount(p,b,SourceRuntimeMemory::Access::ReadOnly,"actual captured source vertex-tag fragment");}
  constexpr unsigned device=0x40f0c0,cache_state=scratch+0x1000;
  memory.mount(0x411568,std::as_bytes(std::span(&device,1)),SourceRuntimeMemory::Access::ReadOnly,"declared CPU device alias");
  memory.mount(device+8,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadOnly,"declared non-pure CPU device flags; no GPU or boot history");
  memory.mount(device+0x10a8,std::vector<std::byte>(136*16,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared CPU program cache workspace");
  put(cache_state+4,0xffffffff);put(cache_state+8,0xffffffff);SourceParticleConstantStorageTu15 storage;storage.begin(memory,inherited);
  for(unsigned row=1;row<=3;++row){put(sp,stop);auto a=abi();const unsigned entry=row==1?0x1c2c0:0x1c570;
   if(row==1){a.gp[0]=7;a.gp[1]=material->vertex.vertex_datum;a.gp[2]=scratch+0x900;}else{put(sp+4,7);a.gp[0]=material->vertex.vertex_datum;a.gp[6]=cache_state;}
   const auto before=a;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Original program lookup/load requires an unowned dependency");};
   const auto export_bank=[&](const char *name){if(out.empty())return;std::ofstream f(out/std::to_string(row)/name,std::ios::binary);memory.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});storage.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});};
   if(!out.empty())std::filesystem::create_directory(out/std::to_string(row));export_bank("before.bin");const auto prior=storage.commands().size();source_run_effect_particle_draw_tu15(storage,entry,a,*a.floating_point,services);export_bank("after.bin");record(out,row,entry,before,a,trace,row==1?0x8c5:0x8d5);
   if(row==1){std::vector<std::byte> original(material->vertex.program.size());memory.read(a.gp[0],original);if(original!=material->vertex.program || Bytes(memory.view(scratch+0x900,4)).u32(0)!=51)throw FormatError("Whole original vertex-tag lookup differs from actual material bytes");}
   else if((row==2 && storage.commands().size()!=852) || (row==3 && storage.commands().size()!=prior))throw FormatError("Whole original cold/repeated load cache behavior differs");
  }
 }
 std::cout<<"PASS original51-instruction program cache/packets, actual resident pipeline, mixed constants and6 atomic packet rejections; optional exact source-tag lookup/cold/hot cache cases\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
