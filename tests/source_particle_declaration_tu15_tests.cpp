#define SDL_MAIN_HANDLED
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_tag_metadata.hpp"
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
}
int main(int argc,char **argv)try {
 if(argc!=4 && argc!=5)throw FormatError("Supply shaders, Backwash map, shared maps and optional new diagnostic output directory");
 if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
 const std::filesystem::path out=argc==5?argv[4]:"";if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier declaration evidence");std::filesystem::create_directories(out);}
 Cache cache(argv[2],argv[3]);const auto render=cache.load_particle_system_render_definition(0xe76105ec,0,0);
 auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(render.particle_datum,15,0,0));
 const auto metadata=cache.load_source_tag_metadata(material->vertex.vertex_datum);
 SourceRuntimeMemory memory;constexpr unsigned scratch=0x60000000,sp=scratch+0x8000,stop=0x60300000,table=0x61000000;
 memory.mount(scratch,std::vector<std::byte>(65536,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared native original declaration callers/context");
 memory.mount(metadata.source_address(),metadata.bytes_at(metadata.source_address(),metadata.size()),SourceRuntimeMemory::Access::ReadOnly,"actual original vertex-tag metadata from Backwash");
 memory.mount(0x4f0414,std::as_bytes(std::span(&table,1)),SourceRuntimeMemory::Access::ReadOnly,"declared native source tag-table interface; no startup history");
 const auto tag_address=metadata.source_address();memory.mount(table+(material->vertex.vertex_datum&65535)*16+8,std::as_bytes(std::span(&tag_address,1)),SourceRuntimeMemory::Access::ReadOnly,"actual selected vertex-tag address fragment; other table fields unavailable");
 const auto put=[&](unsigned p,unsigned v){memory.write(p,std::as_bytes(std::span(&v,1)));};
 const auto abi=[&]{SourcePhysicsWorldRuntimeAbi a{};a.gp={0x12345678,0x87654321,0xabcdef01,0x89abcdef,sp,0x76543210,0x13572468,0x24681357};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};return a;};
 memory.write(scratch+0x100,material->vertex.program);
 auto expected=source_particle_declaration_tu15(memory,material->vertex);if(expected.input_mask!=0x1ff)throw FormatError("Actual particle declaration lost its nine produced inputs");
 auto other=material->vertex;other.program.at(4)^=std::byte{1};rejected([&]{(void)source_particle_declaration_tu15(memory,other);},"Declaration metadata accepted a different selected program");
 std::array<ShaderVector,192> inherited{};for(unsigned n=0;n<192;++n)for(unsigned k=0;k<4;++k)inherited[n][k]=std::bit_cast<float>(0x3e000000u+n*4+k);
 const auto record=[&](unsigned row,unsigned entry,const auto &before,const auto &after,const auto &trace,unsigned mask){if(out.empty())return;std::ofstream(out/std::to_string(row)/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(after)},{"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"defined_flags_mask",mask},{"original_boot_or_vertex_buffer_history_claimed",false},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';};
 SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());auto *window=SDL_CreateWindow("Original particle declaration",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);if(!window)throw FormatError(SDL_GetError());
 {
 Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);
 Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);SourceParticleGraphicsBindingTu15 binding(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());binding.begin(memory,inherited);
 put(sp,stop);auto initialize=abi();initialize.gp[0]=0x11;binding.configure(initialize);
 const std::array<unsigned,3> raw{stop,scratch+0x100,0};memory.write(sp,std::as_bytes(std::span(raw)));auto load=abi();binding.load_program(renderer.particle_program_control(),material,load);
 put(sp,stop);auto a=abi();std::vector<unsigned> trace;SourcePhysicsWorldRuntimeAbi before;const auto first=binding.commands().size();
 const auto export_bank=[&](const char *name){if(out.empty())return;std::ofstream f(out/"0"/name,std::ios::binary);memory.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});binding.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});};
 if(!out.empty())std::filesystem::create_directory(out/"0");SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){if(trace.empty()){before=a;export_bank("before.bin");}trace.push_back(pc);};services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Native declaration reached an unowned owner");};
 binding.configure_program_declaration(a,services);export_bank("after.bin");record(0,0x400dd0,before,a,trace,0x8d5);
 if(binding.program_declaration()!=expected || binding.commands().size()-first!=8 || a.gp[4]!=sp+4)throw FormatError("Original same-kind declaration/start publication differs");
 const auto configuration=binding.program_configuration();auto unchanged=inherited;binding.consume_constants(unchanged);if(std::memcmp(unchanged.data(),inherited.data(),sizeof(inherited)))throw FormatError("Declaration/start packet altered inherited constants");
 const auto malformed=[&](unsigned offset,unsigned value){const auto address=0xf7700000+unsigned(first)+offset;std::array<std::byte,4> saved;binding.read(address,saved);binding.write(address,std::as_bytes(std::span(&value,1)));auto untouched=inherited;rejected([&]{binding.consume_constants(untouched);},"Malformed declaration/start packet was accepted");if(std::memcmp(untouched.data(),inherited.data(),sizeof(inherited)))throw FormatError("Malformed declaration/start packet partially published constants");binding.write(address,saved);};
 malformed(0,0x41ea4);malformed(4,1);
 rejected([&]{binding.configure_program_declaration(a);},"Duplicate declaration publication was accepted");
 std::array<std::byte,4> unknown{};rejected([&]{binding.read(0x4125f8,unknown);},"An unowned program Common word was exposed");rejected([&]{binding.read(0x411570,unknown);},"An unknown declaration dirty-mask reader was accepted");rejected([&]{binding.write(0x40f854,unknown);},"An unknown current-program writer was accepted");rejected([&]{binding.read(0x41260c,unknown);},"A generic packed-declaration reader was accepted");
 put(sp,stop);auto current=abi();current.gp[6]=0x40f0c0;binding.run(0x401020,current);if(current.gp[4]!=sp+4)throw FormatError("The original programmable-kind consumer lost the published header");
 binding.begin(memory,inherited);binding.consume_constants(unchanged);if(std::memcmp(unchanged.data(),inherited.data(),sizeof(inherited)))throw FormatError("A new batch retained a stale declaration packet range");
 if(!out.empty())std::ofstream(out/"program.json")<<Json{{"passed",true},{"vertex_datum",expected.vertex_datum},{"classification",expected.classification},{"input_mask",expected.input_mask},{"elements",expected.elements},{"real_pipeline",configuration.pipeline},{"real_vertex_module",configuration.vertex_module},{"original_declaration_header_published",true},{"original_instruction0_start_packet_consumed",true},{"atomic_malformed_start_cases",2},{"original_vertex_buffer_allocation_or_boot_history_claimed",false},{"gpu_draw_claimed",false},{"full_goal_complete",false}}.dump(2)<<'\n';
 }
 SDL_DestroyWindow(window);SDL_Quit();
 mount_source_particle_declaration_data_tu15(memory);constexpr unsigned device=0x40f0c0,context=scratch+0x1000,output=scratch+0x2000,old_header=scratch+0x3000;
 memory.mount(0x411568,std::as_bytes(std::span(&device,1)),SourceRuntimeMemory::Access::ReadOnly,"declared CPU original device alias");
 memory.mount(0x411570,std::vector<std::byte>(4,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared CPU pending dirty-mask state; no GPU history");
 memory.mount(device+0x794,std::vector<std::byte>(12,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared CPU current program/start state");put(device+0x794,old_header);put(old_header+4,16);
 memory.mount(device+0xb04,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared CPU empty vertex-stream binding");
 memory.mount(0x4125f8,std::vector<std::byte>(280,std::byte{0xa5}),SourceRuntimeMemory::Access::ReadWrite,"declared CPU programmable header/input output workspace");put(0x4125fc,16);
 SourceParticleConstantStorageTu15 storage;storage.begin(memory,inherited);
 for(unsigned row=1;row<=40;++row){
  unsigned entry;put(sp,stop);auto a=abi();
  if(row<=18){entry=0x35780;a.gp[0]=row-1;}
  else if(row<=36){entry=0x35840;a.gp[0]=row-19;}
  else if(row==37){entry=0x1c690;a.gp[2]=context;}
  else if(row==38){entry=0x1c310;put(context+4,material->vertex.vertex_datum);put(context+8,7);put(context+0x80,0);put(context+0x84,0);put(context+0x8c,0x44983a);put(sp+4,context);put(sp+8,output);}
  else{entry=0x1c6f0;put(context+0x84,0);if(row==39)memory.write(context+0x88,std::array<std::byte,1>{std::byte{1}});put(sp+4,context);}
  const auto before=a;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};services.required_owner=[](auto &,const auto &,auto &,const auto &){throw FormatError("Original declaration sequence reached an unowned owner");};
  const auto export_bank=[&](const char *name){if(out.empty())return;std::ofstream f(out/std::to_string(row)/name,std::ios::binary);memory.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});storage.visit_committed([&](unsigned p,std::span<const std::byte>b){emit(f,p,b);});};
  if(!out.empty())std::filesystem::create_directory(out/std::to_string(row));export_bank("before.bin");const auto previous=storage.commands().size();source_run_effect_particle_draw_tu15(storage,entry,a,*a.floating_point,services);export_bank("after.bin");record(row,entry,before,a,trace,(row==37 || row>=39)?0x8c5:0x8d5);
  if(row==38){decltype(expected.elements) actual;memory.read(output,std::as_writable_bytes(std::span(actual)));if(actual!=expected.elements)throw FormatError("Native declaration helper differs from the whole original builder");}
  if((row==39 && storage.commands().size()!=previous+8) || (row==40 && storage.commands().size()!=previous))throw FormatError("Original first/repeated declaration setup emitted the wrong start packets");
 }
 std::cout<<"PASS41 original declaration/type/size/reset/cold/hot calls, actual program header/start consumer, two atomic corruptions and unowned input/device boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
