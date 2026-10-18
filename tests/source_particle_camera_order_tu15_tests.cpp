#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/menu_world_depth.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
template<class F>void rejected(F &&f){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}if(!stopped)throw FormatError("Expected pending-view rejection did not occur");}
Json state(const SourcePhysicsWorldRuntimeAbi&a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
void bank(const std::filesystem::path&p,const SourceRuntimeMemory&m,const SourceParticleGraphicsBindingTu15&binding){std::ofstream f(p,std::ios::binary);const auto emit=[&](unsigned a,std::span<const std::byte>b){unsigned n=b.size();f.write(reinterpret_cast<const char*>(&a),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);};m.visit_committed(emit);binding.visit_committed(emit);if(!f)throw FormatError("Cannot export camera-order evidence");}
}
int main(int argc,char**argv)try {
 if(argc!=4 && argc!=5)throw FormatError("Supply shaders, Backwash map, shared maps and optional new evidence directory");
 if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
 const std::filesystem::path out=argc==5?argv[4]:"";if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve prior camera-order evidence");std::filesystem::create_directories(out);}
 Cache cache(argv[2],argv[3]);const auto definition=cache.load_particle_system_render_definition(0xe76105ec,0,0);
 auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(definition.particle_datum,15,0,0));
 SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());auto*window=SDL_CreateWindow("Original camera then view initialization",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);if(!window)throw FormatError(SDL_GetError());
 {
 Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);
 SourceViewProjectionBank camera;for(unsigned n:{0,5,10,15})camera.projection[n]=1;camera.inverse_view.position={3,-2,1};camera.camera_to_world.position={-3,2,-1};const Vec3 eye=camera.camera_to_world.position;
 ParticleDraw seed;seed.material=material;const auto expected=source_view_shader_bank(camera,1024,eye,{320,0,320,240,1,false,false,0},false);std::copy(expected.begin(),expected.end(),seed.vertex.constants.begin());
 seed.vertex.clip_range={0,16777215,38.39999771118164f,16777215};seed.vertex.surface_fog={640,480,1,1};seed.depth={seed.vertex.clip_range,{0,0,1,1}};
 RendererParticleViewInput input;input.material=material;input.constants=seed.vertex.constants;input.clip_range=seed.vertex.clip_range;input.depth_parameters=seed.depth.parameters;input.surface_fog=seed.vertex.surface_fog;input.rectangle={320,0,320,240};
 unsigned cases=0;
 for(unsigned mode:{0u,1u,2u}) {
  const auto control=renderer.prepare_particle_views(std::span(&input,1));const auto prepared=control.acquire(0);
  SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
  for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,10>{{{0x4920d8,4},{0x4918e4,12},{0x492294,1},{0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4},{0x491e7c,4},{0x4c685c,4},{0x60000000,65536}}})memory.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"declared native camera/pass inputs; original startup and live frame history remain separate");
  const auto put=[&](unsigned p,unsigned value){memory.write(p,std::as_bytes(std::span(&value,1)));};const auto word=[&](unsigned p){return Bytes(memory.view(p,4)).u32(0);};
  const std::array<float,2> planes{.015625f,1024};memory.write(0x4922f4,std::as_bytes(std::span(planes)));put(0x491e7c,0x44800000);put(0x4c685c,17);
  memory.write(0x60000200,std::as_bytes(std::span(&camera,1)));memory.write(0x60000400,std::as_bytes(std::span(&eye,1)));const std::array<std::int16_t,4> rect{0,320,240,640};memory.write(0x60000430,std::as_bytes(std::span(rect)));
  constexpr unsigned sp=0x60008000,stop=0x60300000;put(sp,stop);
  SourceParticleGraphicsBindingTu15 binding(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());binding.begin(memory,seed.vertex.constants);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x11;abi.gp[4]=sp;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};binding.configure(abi);binding.configure_depth(renderer.depth_mode_control());binding.stage_program(renderer.particle_program_control(),material);binding.prepare_view(prepared);
  rejected([&]{(void)prepared.published_state();});std::array<ShaderVector,192> unavailable;rejected([&]{binding.consume_constants(unavailable);});
  const std::array<unsigned,3> args{stop,0,0};memory.write(sp,std::as_bytes(std::span(args)));abi.gp[0]=0x60000200;abi.gp[1]=0x60000400;abi.gp[4]=sp;
  auto before=abi;auto path=out/std::to_string(cases++);std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto pc){trace.push_back(pc);};
  if(!out.empty()){std::filesystem::create_directory(path);bank(path/"before.bin",memory,binding);}binding.publish_camera(abi,services);
  if(!out.empty()){bank(path/"after.bin",memory,binding);std::ofstream(path/"case.json")<<Json{{"entry",0x12f80},{"before",state(before)},{"after",state(abi)},{"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"camera_before_view",true}}.dump(2)<<'\n';}
  rejected([&]{(void)prepared.published_state();});rejected([&]{binding.consume_constants(unavailable);});
  Camera native_camera;ModelView view;view.camera=native_camera;view.left=320;view.width=320;view.height=240;view.full_surface=true;view.world_particles=std::span(&seed,1);
  rejected([&]{(void)renderer.draw(native_camera,{}, {},false,{},{},{},std::span(&view,1));});if(!prepared.valid())throw FormatError("Pending view rejection retired its untouched allocation");
  abi.gp[1]=0x60000100;abi.gp[4]=sp;put(sp,stop);const auto pending=abi;unsigned reset_instructions=0;SourcePhysicsWorldRuntimeServices blocked;blocked.instruction=[&](auto){++reset_instructions;};const auto commands=binding.commands().size();
  put(0x4c2aa0,7);rejected([&]{(void)binding.initialize_prepared_view_pass(mode,0x43480000,abi,blocked);});
  if(word(0x4c2aa0)!=7 || word(0x4c685c)!=17 || reset_instructions || binding.commands().size()!=commands || abi.gp!=pending.gp)throw FormatError("Late queue admission changed state before rejection");
  put(0x4c2aa0,0); // Restore this negative fixture; no live queue is retired here.
  path=out/std::to_string(cases++);if(!out.empty())std::filesystem::create_directory(path);trace.clear();bool observed=false;
  services.instruction=[&](auto pc){if(!observed){if(pc!=0x13430)throw FormatError("Wrong initializer entry");observed=true;before=abi;if(!out.empty())bank(path/"before.bin",memory,binding);}trace.push_back(pc);};
  unsigned missing=0;services.required_owner=[&](auto&,const auto&call,auto&,const auto&){missing=call.target;throw FormatError("Original target/clear owner remains required");};
  std::optional<SourceParticleViewPassTu15> packet;std::string error;
  try{packet=binding.initialize_prepared_view_pass(mode,0x43480000,abi,services);}catch(const FormatError&e){error=e.what();}
  if(!observed || word(0x4c685c)!=18)throw FormatError("Original pass packet did not precede its actual initializer");
  if(!out.empty()){bank(path/"after.bin",memory,binding);std::ofstream(path/"case.json")<<Json{{"entry",0x13430},{"before",state(before)},{"after",state(abi)},{"instructions",trace},{"return_address",mode==2?missing:stop},{"completed",mode!=2},{"missing_owner",missing},{"camera_before_view",true}}.dump(2)<<'\n';}
  if(mode==2){
   if(error.empty() || missing!=0x14ba0 || packet)throw FormatError("Boundary2 concealed its required target owner");
   rejected([&]{(void)prepared.published_state();});reset_instructions=0;
   rejected([&]{binding.initialize_prepared_view_pass(0,0,abi,blocked);});
   rejected([&]{binding.publish_camera(abi,blocked);});
   if(reset_instructions || word(0x4c685c)!=18 || word(0x4c2aa0))throw FormatError("Failed original initializer was retried or changed its queue/counter");
   continue;
  }
  if(!error.empty() || !packet || packet->words!=std::array<unsigned,3>{18,mode,0x43480000})throw FormatError("Complete original pass initializer did not retain its packet");
  const auto clip=menu_world_depth_clip_planes({.015625f,1024.f,mode?MenuWorldDepthBoundary::near_plane:MenuWorldDepthBoundary::none,200,false});auto draw=seed;binding.consume_draw(draw);const auto published=prepared.published_state();
  if(draw.vertex.clip_range!=ShaderVector{0,16777215,clip.minimum_w,clip.maximum_w} || published.vertex_clip!=draw.vertex.clip_range || draw.vertex.constants!=seed.vertex.constants || word(0x4c685c)!=18 || word(0x4c2aa0))throw FormatError("Last original bounds did not reach the published resource");
  if(mode && draw.vertex.clip_range[2]<=255.99998474121094f)throw FormatError("Earlier camera bounds replaced the later boundary initializer");
  rejected([&]{binding.initialize_prepared_view_pass(mode,0,abi);});
  if(!renderer.draw(native_camera,{}, {},false,{},{},{},std::span(&view,1)))throw FormatError("Complete original camera/view did not reach recording");
  const auto actual=renderer.particle_view_inputs().acquire(0);if(actual.state().depth_clip!=draw.depth.clip_range || prepared.valid())throw FormatError("Actual recorded allocation lost the last original bounds or retained stale use");
 }
 }
 SDL_DestroyWindow(window);SDL_Quit();std::cout<<"PASS original camera-before-view order, deferred publication, late-queue rejection and last-writer bounds in actual GPU allocations\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
