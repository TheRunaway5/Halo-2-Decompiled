#include "halo2/hud.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_frustum_camera.hpp"
#include "halo2/source_view_projection.hpp"
#include <SDL_vulkan.h>
#include <vulkan/vulkan.h>
#include <algorithm>
#include <bit>
#include <cstdlib>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
void require(bool value,const std::string& message) {if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);Image image;std::string magic;unsigned maximum;
  file>>magic>>image.width>>image.height>>maximum;file.get();
  require(magic=="P6" && maximum==255,"invalid rectangle framebuffer");
  image.pixels.resize(std::size_t(image.width)*image.height*3);
  file.read(reinterpret_cast<char*>(image.pixels.data()),image.pixels.size());
  require(bool(file),"truncated rectangle framebuffer");return image;
}
std::vector<Vertex> quad(unsigned material,float aspect) {
  std::vector<Vertex> vertices;
  for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
    Vertex v;v.position={2,(1-uv.x*2)*aspect*2,(1-uv.y*2)*2};v.uv=uv;
    v.color={1,1,1};v.normal={-1,0,0};v.tangent={0,-1,0};v.binormal={0,0,-1};v.material=material;
    vertices.push_back(v);
  }
  return vertices;
}
Material planar(Material source) {
  // A controlled flat plane isolates the original stencil_texture position
  // program and copy combiner from geometry/textures elsewhere in the map.
  source.forward_vertex=3;source.program_constants[1][0]={1,1,0,0};
  source.program_constants[1][1]={1,1,0,0};source.forward_transforms[0]={1,1,0,0};
  source.forward_transforms[1]={1,1,0,0};source.forward_raster.depth_enable=0;
  source.forward_raster.depth_write=0;source.forward_raster.cull=0;
  source.forward_raster.color_write=0x10101;source.environment_pass.reset();
  source.world_overlays_before.clear();source.world_overlays_after.clear();
  source.world_bloom_passes.clear();source.world_planar_passes={};
  source.animation.overlays.clear();for(auto& images:source.animated_textures)images.clear();
  return source;
}
Image capture(Renderer& renderer,const std::filesystem::path& path,
              std::span<const ModelView> views) {
  bool drawn=false;for(unsigned attempt=0;attempt<4 && !drawn;++attempt)
    drawn=renderer.draw(views.front().camera,path,{},false,{},{},{},views);
  require(drawn,"rectangle framebuffer was not rendered");return read(path);
}
struct Frames {Image baseline,effects;};
Frames run(Renderer& renderer,SDL_Window* window,std::array<unsigned,2> extent,
           unsigned players,bool wide,const std::filesystem::path& prefix) {
  SDL_SetWindowSize(window,int(extent[0]),int(extent[1]));SDL_PumpEvents();renderer.resized();
  int w,h;SDL_Vulkan_GetDrawableSize(window,&w,&h);
  require(w==int(extent[0]) && h==int(extent[1]),"window did not adopt requested odd drawable extent");
  std::array<ModelView,4> views;std::array<std::vector<Vertex>,4> world,foreground;
  for(unsigned i=0;i<players;++i) {
    const auto r=hud_viewport(players,i,wide).outer;auto& v=views[i];
    v.left=r.left;v.top=r.top;v.width=r.right-r.left;v.height=r.bottom-r.top;v.full_surface=true;
    const auto physical=full_surface_viewport({v.left,v.top,v.width,v.height},extent[0],extent[1]);
    const float aspect=physical.width/physical.height;v.camera.field_of_view=90;
    v.camera.oriented=true;v.camera.forward={1,0,0};v.camera.up={0,0,1};
    SourceFrustumProjectionInput input;input.projection_angle=1.5707963705062866f;
    input.outer=input.safe={std::int16_t(physical.top),std::int16_t(physical.left),
      std::int16_t(physical.top+physical.height),std::int16_t(physical.left+physical.width)};
    input.scenario_present=true;input.scenario_type=2;
    const auto projection=source_frustum_projection(input);
    const auto bank=source_view_projection({{},v.camera.forward,v.camera.up,.015625f,1024.f,{},projection.projection_program});
    SourceViewShaderConstants constants;constants.world=source_view_shader_constants(bank,1024,false);
    constants.reflection=constants.world;const auto fg=source_view_shader_constants(bank,1024,true);
    std::copy_n(fg.begin(),4,constants.foreground.begin());v.source_view_shader_constants=constants;
    world[i]=quad(i,aspect);foreground[i]=quad(4,aspect);v.vertices=world[i];
    v.atmosphere=BspFogSample{};v.atmosphere->bloom={1,0,0,0};
  }
  Frames result;result.baseline=capture(renderer,prefix.string()+"-baseline.ppm",{views.data(),players});
  for(unsigned i=0;i<players;++i) {
    auto& v=views[i];const auto physical=full_surface_viewport({v.left,v.top,v.width,v.height},extent[0],extent[1]);
    v.atmosphere->bloom={1,0,1,0};v.foreground=foreground[i];
    v.scope=scope_copy_frame(scope_source_rectangle(physical,{.35f,.3f},scope_sample_center(physical),
      {float(extent[0]),float(extent[1])}));
  }
  result.effects=capture(renderer,prefix.string()+"-effects.ppm",{views.data(),players});
  require(result.effects.width==extent[0] && result.effects.height==extent[1],"capture retained stale extent");
  for(unsigned i=0;i<players;++i) {
    const auto& v=views[i];const auto p=full_surface_viewport({v.left,v.top,v.width,v.height},extent[0],extent[1]);
    const unsigned l=unsigned(p.left),t=unsigned(p.top),right=l+unsigned(p.width),bottom=t+unsigned(p.height);
    const auto center=(std::size_t(t+unsigned(p.height)/2)*extent[0]+l+unsigned(p.width)/2)*3;
    require(result.effects.pixels[center]>result.baseline.pixels[center]+5,
      "controlled source scope/bloom passes were not exercised");
    // Uniform input and clamped per-view intermediates must process the last
    // row/column as well. This fails the former independently rounded size.
    const auto check=[&](unsigned x,unsigned y) {
      const auto at=(std::size_t(y)*extent[0]+x)*3;
      for(unsigned c=0;c<3;++c)require(std::abs(int(result.effects.pixels[at+c])-int(result.effects.pixels[center+c]))<=2,
        "effect skipped or crossed a source integer view boundary at "+std::to_string(x)+","+std::to_string(y));
    };
    for(unsigned x=l+8;x+8<right;x+=13)check(x,bottom-1);
    for(unsigned y=t+8;y+8<bottom;y+=13)check(right-1,y);
  }
  return result;
}
}
int main(int argc,char**argv) {try {
  require(argc==3,"rectangle GPU test requires original maps and output directory");
#if defined(__linux__)
  if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))return 77;
#endif
  const Cache cache(std::filesystem::path(argv[1])/"lockout.map");
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto& t) {
    return t.group=="shad" && t.name=="objects\\weapons\\rifle\\sniper_rifle\\shaders\\sniper_rifle_screen";
  });
  require(tag!=cache.tags().end(),"missing authored scope shader");auto root=cache.load_material(tag->datum);
  require(prepare_dynamic_material(cache,root,{},true),"authored scope shader was not admitted");
  const auto scope=std::find_if(root.world_overlays_after.begin(),root.world_overlays_after.end(),[](const auto& m) {
    return m && m->scope_texture;
  });require(scope!=root.world_overlays_after.end(),"missing source target16 consumer");
  const auto source=planar(**scope);Mesh mesh;mesh.vertices.resize(3);
  for(unsigned i=0;i<4;++i) {
    auto m=source;m.scope_texture=false;m.forward_program=scope_copy_frame({}).program;m.forward_blend=0;
    m.diffuse={1,1,11,{std::byte(24+i*8),std::byte(20+i*8),std::byte(16+i*8),std::byte{255}}, {}};
    mesh.materials.push_back(std::move(m));
  }
  auto consumer=source;consumer.detail={1,1,11,std::vector<std::byte>(4,std::byte{255}),{}};
  require(consumer.forward_blend==3,"source scope additive blend changed");mesh.materials.push_back(consumer);
  const auto atmosphere=cache.load_bsp_atmosphere(0);const auto directory=std::filesystem::path(argv[2]);
  std::filesystem::create_directories(directory);
  require(SDL_Init(SDL_INIT_VIDEO)==0,"SDL rectangle init failed");
  unsigned layer_count=0;vkEnumerateInstanceLayerProperties(&layer_count,nullptr);
  std::vector<VkLayerProperties> layers(layer_count);vkEnumerateInstanceLayerProperties(&layer_count,layers.data());
  if(std::none_of(layers.begin(),layers.end(),[](const auto& l){return std::string(l.layerName)=="VK_LAYER_KHRONOS_validation";})) {
    SDL_Quit();return 77;
  }
  auto* reused_window=SDL_CreateWindow("Source rectangle resize",0,0,640,480,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
  require(reused_window,"SDL rectangle window failed");std::size_t channels=0;
  {
    Renderer reused(reused_window,mesh,true);reused.scene_bsp_atmosphere(atmosphere);
    // Alternate sizes deliberately: repeated odd sizes also follow 4K and
    // even sizes, exercising existing resource teardown/recreation paths.
    for(const auto extent:std::array{std::array{853u,479u},std::array{1280u,720u},
        std::array{1281u,721u},std::array{3840u,2160u},std::array{1100u,651u},std::array{853u,479u}})
      for(unsigned players:{1u,2u,3u,4u}) {
        const auto name=std::to_string(extent[0])+"x"+std::to_string(extent[1])+"-"+std::to_string(players);
        const auto prior=run(reused,reused_window,extent,players,true,directory/(name+"-reused"));
        auto* fresh_window=SDL_CreateWindow("Fresh source rectangle",0,0,int(extent[0]),int(extent[1]),SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
        require(fresh_window,"fresh SDL rectangle window failed");
        {
          Renderer fresh(fresh_window,mesh,true);fresh.scene_bsp_atmosphere(atmosphere);
          const auto next=run(fresh,fresh_window,extent,players,true,directory/(name+"-fresh"));
          require(prior.baseline.pixels==next.baseline.pixels && prior.effects.pixels==next.effects.pixels,
            "fresh/reused source rectangle or effect images differ after resize");
          channels+=prior.effects.pixels.size()*2;
        }
        SDL_DestroyWindow(fresh_window);
      }
  }
  SDL_DestroyWindow(reused_window);SDL_Quit();
  std::cout<<channels<<" fresh/reused source-camera/scope/bloom channel checks and all submitted last rows/columns passed\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';SDL_Quit();return 1;}}
