#define SDL_MAIN_HANDLED
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
template<class F>void rejected(F &&f,const char *why){bool stopped=false;try{f();}catch(const std::exception &){stopped=true;}if(!stopped)throw FormatError(why);}
Json state(const SourcePhysicsWorldRuntimeAbi &a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
}
int main(int argc,char **argv)try {
 if(argc!=4 && argc!=5)throw FormatError("Supply shaders, Backwash map, shared maps and optional new diagnostic directory");
 if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
 const std::filesystem::path out=argc==5?argv[4]:"";if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve earlier program binding evidence");std::filesystem::create_directories(out);}
 Cache cache(argv[2],argv[3]);const auto render=cache.load_particle_system_render_definition(0xe76105ec,0,0);
 auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(render.particle_datum,15,0,0));
 SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());auto *window=SDL_CreateWindow("Original particle program owner",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);if(!window)throw FormatError(SDL_GetError());
 SourceRuntimeMemory memory;memory.mount(0x60000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"declared native program caller stack");
 const unsigned stop=0x60300000;memory.write(0x60008000,std::as_bytes(std::span(&stop,1)));std::array<ShaderVector,192> inherited{};
 std::optional<SourceParticleGraphicsBindingTu15> binding;RendererParticleProgram retained_control;
 {
 Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);
 Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);auto targets=renderer.target_resources();auto control=renderer.particle_program_control();retained_control=control;
 rejected([&]{(void)control.select({});},"Null material was accepted");auto changed=std::make_shared<ParticleMaterial>(*material);changed->vertex.program.at(4)^=std::byte{1};
 rejected([&]{(void)control.select(changed);},"Mutated original shader tokens were accepted");
 binding.emplace(renderer.device_lease(),renderer.constant_mode_control(),targets.acquire());binding->begin(memory,inherited);
 SourcePhysicsWorldRuntimeAbi initialize{};initialize.gp[0]=0x11;initialize.gp[4]=0x60008000;initialize.eflags=0x202;initialize.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};binding->configure(initialize);
 rejected([&]{(void)binding->program_configuration();},"Unstaged program became visible");rejected([&]{binding->stage_program({},material);},"Unissued program control was accepted");
 binding->stage_program(control,material);const auto config=binding->program_configuration();
 if(!config.pipeline || !config.vertex_module || !config.layout || config.state_backing.value<=0xffffffffu || config.classification!=7 || config.instructions!=51 || config.target_generation!=targets.acquire().bank().generation)
  throw FormatError("Original program does not have its actual class7 pipeline and51 instructions");
 const auto again=control.select(material);if(again.pipeline!=config.pipeline || again.vertex_module!=config.vertex_module || again.state_backing!=config.state_backing)throw FormatError("Native resident program identity was not stable");
 rejected([&]{binding->stage_program(control,material);},"Duplicate native program staging was accepted");
 std::array<std::byte,4> bytes{};rejected([&]{binding->read(0x40f854,bytes);},"An unowned program pointer reader was accepted");
 rejected([&]{binding->read(0xf90003c4,bytes);},"An unowned header reader was accepted");rejected([&]{binding->read(0xf90003d0,bytes);},"A fake zero input declaration was exposed");
 rejected([&]{binding->write(0x40f854,bytes);},"Source code replaced a staged native program");
 bool foreign=false;std::thread worker([&]{try{(void)control.select(material);}catch(const std::exception &){foreign=true;}});worker.join();if(!foreign)throw FormatError("Program selection crossed issuing threads");
 const auto export_bank=[&](const std::filesystem::path &path){std::ofstream f(path,std::ios::binary);const auto emit=[&](unsigned p,std::span<const std::byte>b){const unsigned n=b.size();f.write(reinterpret_cast<const char*>(&p),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);};memory.visit_committed(emit);binding->visit_committed(emit);};
 for(unsigned test=0;test<2;++test) {
  SourcePhysicsWorldRuntimeAbi a{};a.gp={0xf7700000,0x12345678,0xabcdef01,0x89abcdef,0x60008000,0x76543210,0x40f0c0,0x24681357};a.eflags=0x202;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const auto before=a;std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto p){trace.push_back(p);};
  const auto path=out/std::to_string(test);if(!out.empty()){std::filesystem::create_directory(path);export_bank(path/"before.bin");}
  bool complete=true;std::string error;const unsigned entry=test?0x400270:0x401020;
  try{binding->run(entry,a,services);}catch(const FormatError &e){complete=false;error=e.what();}
  if(!test && (!complete || trace.size()!=4 || a.gp[4]!=0x60008004 || a.gp[0]!=0xf90003c0))throw FormatError("Whole401020 did not consume the actual programmable kind");
  if(test && (complete || trace.back()!=0x400145 || error.find("40ffa0")==std::string::npos))throw FormatError("Original programmable viewport branch did not reach its actual missing viewport input");
  if(!out.empty()){export_bank(path/"after.bin");std::ofstream(path/"case.json")<<Json{{"entry",entry},{"before",state(before)},{"after",state(a)},{"instructions",complete?trace:std::vector<unsigned>(trace.begin(),trace.end()-1)},{"return_address",complete?stop:trace.back()},{"completed",complete},{"missing_owner",0},{"error",error},{"native_program_interface_declared",true},{"original_sdk_boot_or_current_shader_history_claimed",false},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()}}.dump(2)<<'\n';}
 }
 if(!out.empty())std::ofstream(out/"program.json")<<Json{{"pipeline",config.pipeline},{"vertex_module",config.vertex_module},{"layout",config.layout},{"program_hash",config.program_hash},{"classification",config.classification},{"instructions",config.instructions},{"target_generation",config.target_generation},{"state_backing",config.state_backing.value},{"device_name",renderer.device_name()},{"gpu_draw_claimed",false},{"full_goal_complete",false}}.dump(2)<<'\n';
 const auto renewed=targets.source_buffers(2);(void)renewed;rejected([&]{(void)binding->program_configuration();},"A recreated target retained its staged program view");
 }
 if(retained_control.valid() || binding->configured())throw FormatError("Retired renderer retained its particle program");
 rejected([&]{(void)retained_control.select(material);},"Retired program remained selectable");binding.reset();SDL_DestroyWindow(window);SDL_Quit();
 std::cout<<"PASS actual original class7 program residency, programmable-kind CPU consumers, unknown/mutated/duplicate/thread/generation/retirement boundaries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
