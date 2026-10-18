#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/particle_render_inputs.hpp"
#include "halo2/source_effect_particle_draw_tu15.hpp"
#include "halo2/source_particle_graphics_binding_tu15.hpp"
#include "halo2/source_view_projection.hpp"
#include <SDL.h>
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;using Json=nlohmann::json;
namespace {
template<class F>void rejected(F &&f,const char *why){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}if(!stopped)throw FormatError(why);}
Json state(const SourcePhysicsWorldRuntimeAbi&a){return Json{{"gp",a.gp},{"xmm",a.xmm},{"eflags",a.eflags},{"fs_base",a.fs_base},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr}};}
}
int main(int argc,char**argv)try {
 if(argc!=4 && argc!=5)throw FormatError("Supply shaders, Backwash map, shared maps and optional new evidence directory");
 if(!std::filesystem::exists(std::filesystem::path(argv[1])/"particle.vert.spv") || !std::filesystem::exists(argv[2]))return 77;
 const std::filesystem::path out=argc==5?argv[4]:"";if(!out.empty()){if(std::filesystem::exists(out))throw FormatError("Preserve prior camera publisher evidence");std::filesystem::create_directories(out);}
 Cache cache(argv[2],argv[3]);const auto render=cache.load_particle_system_render_definition(0xe76105ec,0,0);
 auto material=std::make_shared<const ParticleMaterial>(cache.load_particle_material(render.particle_datum,15,0,0));
 SDL_SetMainReady();if(SDL_Init(SDL_INIT_VIDEO))throw FormatError(SDL_GetError());auto*window=SDL_CreateWindow("Original camera constant publisher",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);if(!window)throw FormatError(SDL_GetError());
 {
 Mesh mesh;Material m;m.diffuse={1,1,11,{std::byte{32},std::byte{80},std::byte{128},std::byte{255}}};mesh.materials.push_back(m);
 Renderer renderer(window,mesh,true,{},nullptr,{},nullptr,argv[1]);
 // Declared camera fixture. Whole12F80 must reproduce these actual mapped
 // GPU inputs; no source camera constructor/frame history is claimed.
 SourceViewProjectionBank camera;for(unsigned n:{0,5,10,15})camera.projection[n]=1;
 camera.inverse_view.position={3,-2,1};camera.camera_to_world.position={-3,2,-1};
 const Vec3 eye=camera.camera_to_world.position;ParticleDraw draw;draw.material=material;
 const auto expected=source_view_shader_bank(camera,1024,eye,{320,0,320,240,1,false,false,0},false);
 std::copy(expected.begin(),expected.end(),draw.vertex.constants.begin());
 draw.vertex.clip_range={0,16777215,38.39999771118164f,16777215};draw.depth={draw.vertex.clip_range,{0,0,1,1}};
 draw.vertex.surface_fog={640,480,1,1};
 Camera native_camera;ModelView view;view.camera=native_camera;view.left=320;view.width=320;view.height=240;view.full_surface=true;view.world_particles=std::span(&draw,1);
 RendererParticleViewInput input_view;input_view.material=material;input_view.constants=draw.vertex.constants;
 input_view.clip_range=draw.vertex.clip_range;input_view.depth_parameters=draw.depth.parameters;
 input_view.surface_fog={640,480,1,1};input_view.rectangle={320,0,320,240};
 std::array scope_inputs{input_view,input_view};
 rejected([&]{(void)renderer.prepare_particle_views(scope_inputs);},"Duplicate local-view scope was accepted");
 scope_inputs[1].foreground=true;auto scopes=renderer.prepare_particle_views(scope_inputs);
 auto world_scope=scopes.acquire(0),foreground_scope=scopes.acquire(1);
 if(world_scope.state().foreground || !foreground_scope.state().foreground ||
    world_scope.state().uniform_backing==foreground_scope.state().uniform_backing)
  throw FormatError("Prepared world and foreground contexts do not have distinct actual allocations");
 auto view_control=renderer.prepare_particle_views(std::span(&input_view,1));auto prepared=view_control.acquire(0);const auto seed_state=prepared.state();
 if(world_scope.valid() || foreground_scope.valid())throw FormatError("Repreparation retained an earlier view scope");
 bool foreign_rejected=false;std::thread worker([&]{try{(void)renderer.prepare_particle_views(std::span(&input_view,1));}catch(const std::exception&){foreign_rejected=true;}});worker.join();
 if(!foreign_rejected || !prepared.valid())throw FormatError("Prepared allocation crossed the renderer's owning thread");
 if(!prepared.state().prepared || prepared.state().recorded || prepared.state().published)
  throw FormatError("Camera resources were not prepared before any particle command recording");
 rejected([&]{(void)prepared.published_state();},"Unpublished camera resource was readable as produced data");
 std::optional<ParticleDraw> submitted;
 for(unsigned row=0;row<2;++row) {
  SourceRuntimeMemory memory;mount_source_particle_graphics_layout_data_tu15(memory);
  for(const auto[p,n]:std::array<std::pair<unsigned,unsigned>,8>{{{0x4918e4,12},{0x492294,1},{0x4922f4,16},{0x4c2aa0,32},{0x4c4b64,4},{0x4c515c,4},{0x491e7c,4},{0x60000000,65536}}})memory.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,"declared native camera caller and pre-collection view fixture");
  const std::array<float,2> planes{.015625f,1024};memory.write(0x4922f4,std::as_bytes(std::span(planes)));memory.write(0x491e7c,std::as_bytes(std::span(&planes[1],1)));
  const std::array<unsigned,3> frame{17,0,0};memory.write(0x60000100,std::as_bytes(std::span(frame)));
  auto input=camera;if(row==0)input.projection[0]+=1;memory.write(0x60000200,std::as_bytes(std::span(&input,1)));
  memory.write(0x60000400,std::as_bytes(std::span(&eye,1)));const std::array<std::int16_t,4> rect{0,320,240,640};memory.write(0x60000430,std::as_bytes(std::span(rect)));
  constexpr unsigned sp=0x60008000,stop=0x60300000;memory.write(sp,std::as_bytes(std::span(&stop,1)));
  SourceParticleGraphicsBindingTu15 binding(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());binding.begin(memory,draw.vertex.constants);
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[0]=0x11;abi.gp[4]=sp;abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};binding.configure(abi);
  binding.configure_depth(renderer.depth_mode_control());binding.stage_program(renderer.particle_program_control(),material);
  abi.gp[1]=0x60000100;abi.gp[4]=sp;binding.initialize_view(renderer.particle_view_inputs().acquire(0),abi);
  const unsigned occupied=0;memory.write(0x4c2aa0,std::as_bytes(std::span(&occupied,1)));
  const std::array<unsigned,3> args{stop,0,0};memory.write(sp,std::as_bytes(std::span(args)));abi.gp[0]=0x60000200;abi.gp[1]=0x60000400;abi.gp[4]=sp;const auto before=abi;
  const auto path=out/std::to_string(row);if(!out.empty())std::filesystem::create_directory(path);
  const auto bank=[&](const char*name){std::ofstream f(path/name,std::ios::binary);const auto emit=[&](unsigned p,std::span<const std::byte>b){unsigned n=b.size();f.write(reinterpret_cast<const char*>(&p),4);f.write(reinterpret_cast<const char*>(&n),4);f.write(reinterpret_cast<const char*>(b.data()),n);};memory.visit_committed(emit);binding.visit_committed(emit);};
  if(!out.empty())bank("before.bin");std::vector<unsigned> trace;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](unsigned pc){trace.push_back(pc);};services.required_owner=[](auto&,const auto&,auto&,const auto&){throw FormatError("Camera reached an unowned original service");};
  std::string error;try{binding.publish_camera(abi,services);}catch(const FormatError&e){error=e.what();}
  if(row==1) {
   if(!error.empty())throw FormatError(error);auto produced=draw;binding.consume_draw(produced);
   submitted=produced;const auto published=prepared.published_state();
   if(!published.prepared || !published.published || published.recorded || published.uniform_backing!=prepared.state().uniform_backing ||
      published.camera_constants!=prepared.state().camera_constants || published.vertex_clip!=produced.vertex.clip_range)
    throw FormatError("Original camera did not publish into the actual pre-collection allocation");
   if(produced.vertex.constants!=draw.vertex.constants || produced.vertex.attributes!=draw.vertex.attributes ||
      produced.vertex.clip_range[2]!=255.99998474121094f || produced.depth.clip_range[2]!=255.99998474121094f)
    throw FormatError("Original prepared camera failed to publish its actual source clip change");
   rejected([&]{binding.publish_camera(abi);},"Duplicate camera publication was accepted");
  }else {
   if(error.find("descriptor camera bank")==std::string::npos)throw FormatError("Mismatched original camera was not rejected by its native GPU input owner");
   auto destination=draw.vertex.constants;const auto original=destination;rejected([&]{binding.consume_constants(destination);},"Rejected camera published constants");
   if(destination!=original)throw FormatError("Rejected camera partially changed its destination");
  }
  if(Bytes(memory.view(0x4c2aa0,4)).u32(0)!=occupied || abi.gp[4]!=sp+12)throw FormatError("Whole camera publisher changed the queue or return ABI");
  if(!out.empty()){bank("after.bin");std::ofstream(path/"case.json")<<Json{{"entry",0x12f80},{"before",state(before)},{"after",state(abi)},{"instructions",trace},{"return_address",stop},{"completed",true},{"missing_owner",0},{"error",error},{"native_camera_accepted",row==1},{"command_bytes",binding.commands().size()},{"game_returns_supplied",Json::array()},{"gpu_returns_supplied",Json::array()},{"original_camera_history_claimed",false}}.dump(2)<<'\n';}
 }
 if(!submitted)throw FormatError("No complete original camera publication");
 SourceRuntimeMemory sealed;sealed.mount(0x60000000,std::vector<std::byte>(65536),SourceRuntimeMemory::Access::ReadWrite,"declared camera-reuse rejection stack");
 const unsigned stop=0x60300000;sealed.write(0x60008000,std::as_bytes(std::span(&stop,1)));
 SourceParticleGraphicsBindingTu15 duplicate(renderer.device_lease(),renderer.constant_mode_control(),renderer.target_resources().acquire());
 duplicate.begin(sealed,draw.vertex.constants);SourcePhysicsWorldRuntimeAbi duplicate_abi{};duplicate_abi.gp[0]=0x11;duplicate_abi.gp[4]=0x60008000;
 duplicate_abi.eflags=0x202;duplicate_abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};duplicate.configure(duplicate_abi);
 duplicate.configure_depth(renderer.depth_mode_control());duplicate.stage_program(renderer.particle_program_control(),material);
 unsigned duplicate_instructions=0;SourcePhysicsWorldRuntimeServices duplicate_services;duplicate_services.instruction=[&](auto){++duplicate_instructions;};
 rejected([&]{duplicate.initialize_view(prepared,duplicate_abi,duplicate_services);},"Published camera resource was reinitialized through an older input snapshot");
 if(duplicate_instructions)throw FormatError("Published camera rejection ran original reset instructions");
 auto wrong=*submitted;wrong.vertex.constants[0][0]+=1;view.world_particles=std::span(&wrong,1);
 rejected([&]{(void)renderer.draw(native_camera,{}, {},false,{},{},{},std::span(&view,1));},"Mismatched collected camera was accepted");
 if(!prepared.valid())throw FormatError("Rejected input retired the untouched prepared allocation");
 view.world_particles=std::span(&draw,1);const auto saved_left=view.left;view.left=0;
 rejected([&]{(void)renderer.draw(native_camera,{}, {},false,{},{},{},std::span(&view,1));},"Mismatched view rectangle was accepted");view.left=saved_left;
 if(!prepared.valid())throw FormatError("Rejected view rectangle retired the untouched prepared allocation");
 if(!renderer.draw(native_camera,{}, {},false,{},{},{},std::span(&view,1)))throw FormatError("Prepared camera did not reach an actual particle draw");
 if(prepared.valid())throw FormatError("Replaced draw bank retained a prepared camera use");
 rejected([&]{(void)prepared.published_state();},"Replaced camera uniform allocation remained readable");
 const auto recorded=renderer.particle_view_inputs().acquire(0);const auto &actual=recorded.state();
 if(!actual.recorded || actual.prepared || actual.camera_constants!=seed_state.camera_constants ||
    actual.depth_clip!=submitted->depth.clip_range || actual.depth_clip==draw.depth.clip_range)
  throw FormatError("Actual draw did not retain its source camera inputs");
 }
 SDL_DestroyWindow(window);SDL_Quit();std::cout<<"PASS actual pre-collection camera allocation, complete original publication, explicit view/foreground ownership, mismatched camera/scissor rejection, same-frame GPU transfer and resource expiry\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
