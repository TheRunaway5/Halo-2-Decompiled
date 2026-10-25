#include "halo2/renderer.hpp"
#include "halo2/source_objective_presentation_tu15.hpp"
#include "halo2/source_king_draw.hpp"
#include "halo2/source_transparent_queue.hpp"
#include "halo2/source_transparent_parts.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include "halo2/world_material.hpp"
#include "halo2/shader_animation.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool pass,const std::string& why){if(!pass)throw FormatError(why);}
std::vector<std::uint8_t> bytes(const std::string& hex){std::vector<std::uint8_t> out;for(unsigned i=0;i<hex.size();i+=2)out.push_back(std::stoul(hex.substr(i,2),nullptr,16));return out;}
std::vector<unsigned char> pixels(const std::filesystem::path& path) {
 std::ifstream file(path,std::ios::binary);std::string magic;unsigned width,height,max;file>>magic>>width>>height>>max;file.get();
 check(magic=="P6" && width==640 && height==480 && max==255,"Objective goal renderer capture extent differs");
 std::vector<unsigned char> out(width*height*3);file.read(reinterpret_cast<char*>(out.data()),out.size());check(bool(file),"Truncated objective goal capture");return out;
}
unsigned changed(const auto& a,const auto& b){unsigned n{};for(unsigned i=0;i<a.size();++i)n+=a[i]!=b[i];return n;}
}
int main(int argc,char** argv)try {
 check(argc==2 && std::getenv("HALO2_TEST_SCRATCH"),"Supply MCC map inside the owned test lifecycle");const std::filesystem::path output=std::filesystem::path(std::getenv("HALO2_TEST_SCRATCH"))/"objective-goal-renderer";std::filesystem::create_directories(output);
 Cache cache(argv[1]);check(cache.is_mcc(),"Goal runtime test requires MCC resource ownership");const auto shader=cache.load_multiplayer_hill_shader();
 auto material=cache.load_material(shader);check(prepare_simple_world_material(cache,material),"Actual selected objective goal pass unavailable");
 const auto metadata=cache.load_source_transparent_shader(shader);check(metadata.template_lod_layers.size()==1 && metadata.template_lod_layers[0]==0x8000,"Objective goal LOD/layer contract differs");
 SourceObjectiveGoalTu15 goal{shader,0,{0,0,0},{.5f,.5f,.5f},1.f,1.5f};
 const std::array<float,3> eye{-2,0,.4f};
 auto boundary=source_objective_goal_boundary_tu15(goal,{eye[0],eye[1],eye[2]},10117,30);
 check(bool(boundary),"Visible original objective goal boundary unavailable");
 const auto triangles=source_king_boundary_triangles(*boundary,0);auto zero=triangles,full=triangles;
 for(auto& v:zero)v.source_color_alpha=0;
 for(auto& v:full)v.source_color_alpha=1;
 SourceTransparentQueue queue;SourceTransparentShaderInput submission;
 submission.selected_template_layers=metadata.template_lod_layers[0];submission.shader=shader;submission.context=0;submission.callback=0x24d760;
 submission.shader_priority_flags=metadata.shader_priority_flags;submission.center=goal.center;
 submission.camera_position={eye[0],eye[1],eye[2]};submission.camera_forward={1,0,0};
 check(queue.append_shader(submission) && queue.count()==1,"Original objective goal submission did not enter actual shared queue");
 SourceTransparentDrawBinding binding=SourceTransparentShaderGeometryDraw{0x24d760,{shader,0u,triangles,{},{}}};
 ModelView view;view.camera.position=submission.camera_position;view.camera.oriented=true;view.camera.forward={1,0,0};view.camera.up={0,0,1};view.camera.field_of_view=90;
 view.width=640;view.height=480;view.full_surface=true;view.effect_time=10117.f/30;
 view.atmosphere=BspFogSample{};view.atmosphere->clear=Vec3{};
 SourceFrustumProjectionInput frustum;frustum.projection_angle=1.5707963267948966f;frustum.outer=frustum.safe={0,0,480,640};
 SourceViewProjectionInput camera;camera.position=view.camera.position;camera.forward={1,0,0};camera.up={0,0,1};camera.near_distance=.015625f;camera.far_distance=1024;camera.projection_program=source_frustum_projection(frustum).projection_program;
 const auto bank=source_view_projection(camera);SourceViewShaderConstants constants;
 constants.world=source_view_shader_constants(bank,1024,false);
 const auto foreground=source_view_shader_constants(bank,1024,true);std::copy_n(foreground.begin(),4,constants.foreground.begin());
 constants.reflection=constants.world;view.source_view_shader_constants=constants;
 std::vector<SourceTransparentDrawBinding> bindings{binding};view.world_transparent=SourceTransparentDrawBank{queue.commands(),0,bindings};
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto* window=SDL_CreateWindow("Authored Assault goal boundary",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);check(window,SDL_GetError());
 Mesh mesh;mesh.vertices.resize(3);mesh.materials.push_back(material);std::ostringstream diagnostics;auto* previous=std::cerr.rdbuf(diagnostics.rdbuf());
 try {
  Renderer renderer(window,mesh,!std::getenv("HALO2_OBJECTIVE_NO_VALIDATION"));renderer.scene_bsp_atmosphere(cache.load_bsp_atmosphere(0));
  const auto capture=[&](const char* name,std::span<const Vertex> geometry,float time) {
   std::get<SourceTransparentShaderGeometryDraw>(bindings[0]).geometry.vertices=geometry;view.effect_time=time;
   (void)animate_forward_material(mesh.materials[0],time,{});renderer.update_material_constants(mesh.materials);
   const auto path=output/name;for(unsigned attempt=0;attempt<5;++attempt)if(renderer.draw(view.camera,path,{},false,{},{},{},std::span{&view,1}))return pixels(path);
   throw FormatError("Objective goal renderer frame did not submit");
  };
  const auto empty=capture("empty.ppm",{},10117.f/30),transparent=capture("alpha-zero.ppm",zero,10117.f/30);
  const auto authored=capture("authored.ppm",triangles,10117.f/30),opaque=capture("alpha-one.ppm",full,10117.f/30);
  const auto animated=capture("animated.ppm",triangles,10117.f/30+.5f);
  check(empty==transparent,"Zero source RGBA11 alpha survived original alpha test");
  check(changed(empty,authored)>100,"Authored objective goal produced no visible host pixels");
  check(changed(authored,opaque)>100,"Per-vertex source opacity was lost in physical vertex input");
  check(changed(authored,animated)>100,"Actual objective goal overlay animation did not reach the renderer");
  auto& selected=std::get<SourceTransparentShaderGeometryDraw>(bindings[0]);selected.callback=0x12345678;
  bool rejected=false;try{renderer.draw(view.camera,{}, {},false,{},{},{},std::span{&view,1});}catch(const FormatError&){rejected=true;}
  check(rejected,"Mismatched source shader callback identity was admitted");
  std::cout<<changed(empty,authored)<<" authored objective goal host pixel channels, "<<changed(authored,opaque)<<" opacity-dependent and "<<changed(authored,animated)<<" animated channels\n";
 }catch(...){std::cerr.rdbuf(previous);std::cerr<<diagnostics.str();SDL_DestroyWindow(window);SDL_Quit();throw;}
 std::cerr.rdbuf(previous);SDL_DestroyWindow(window);SDL_Quit();check(diagnostics.str().empty(),diagnostics.str());
 std::cout<<"Actual MCC objective goal shader, native32-point circle and shared sorted callback render with declared validation availability; declared host view, no original Xbox pixel comparison\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
