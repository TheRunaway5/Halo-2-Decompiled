#define SDL_MAIN_HANDLED
#include "halo2/player_armor_emblem.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include "halo2/source_dynamic_model_shaders.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/math.hpp"
#include "halo2/renderer.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);std::string magic;Image image;unsigned maximum;
  file>>magic>>image.width>>image.height>>maximum;file.get();
  check(magic=="P6" && maximum==255,"Invalid armor capture");
  image.pixels.resize(std::size_t(image.width)*image.height*3);
  file.read(reinterpret_cast<char*>(image.pixels.data()),image.pixels.size());
  check(bool(file),"Incomplete armor capture");return image;
}
unsigned differences(const Image &a,const Image &b) {
  check(a.width==b.width && a.height==b.height,"Armor capture dimensions changed");
  unsigned pixels=0;
  for(std::size_t i=0;i<a.pixels.size();i+=3)
    pixels+=!std::equal(a.pixels.begin()+i,a.pixels.begin()+i+3,b.pixels.begin()+i);
  return pixels;
}
unsigned changed_color(const Image &a,const Image &b,std::uint32_t rgb) {
  unsigned pixels=0;
  for(std::size_t i=0;i<b.pixels.size();i+=3)
    pixels+=b.pixels[i]==((rgb>>16)&255) && b.pixels[i+1]==((rgb>>8)&255) &&
        b.pixels[i+2]==(rgb&255) &&
        !std::equal(a.pixels.begin()+i,a.pixels.begin()+i+3,b.pixels.begin()+i);
  return pixels;
}
Image capture(Renderer &renderer,Mesh &mesh,const Camera &camera,const std::filesystem::path &path) {
  renderer.update_material_constants(mesh.materials);
  ModelView view;view.camera=camera;view.width=640;view.height=480;
  view.vertices=mesh.vertices;view.full_surface=true;
  for(unsigned i=0;i<4;++i)
    if(renderer.draw(camera,path,{},false,{},{},{},{&view,1}))return read(path);
  throw FormatError("Armor inspection did not submit a Vulkan frame");
}
}
int main(int argc,char **argv)try {
  check(argc==3,"Supply repository and output directories");
  if(!std::getenv("DISPLAY"))return 77;
  SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
  auto *window=SDL_CreateWindow("Original armor emblem asset inspection",0,0,640,480,
      SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);check(window,SDL_GetError());
  const std::filesystem::path root=argv[1],output=argv[2];
  std::filesystem::create_directories(output);
  Cache cache(root/"assets/disc/maps/lockout.map");
  const auto content=cache.load_content(0,true);
  const auto palette=cache.load_full_scoreboard_assets().palette;
  for(unsigned representation:{2u,3u}) {
    const auto unit=content.player_representations.at(representation).biped.datum;
    auto model=cache.load_model(unit,true);
    ObjectLightingState light;light.ambient={.5f,.5f,.5f};
    light.primary_color={.7f,.7f,.7f};light.primary_direction={0,0,1};light.accuracy=1;
    for(auto &material:model.mesh.materials)if(!material.lods.empty())
      check(prepare_dynamic_material(cache,material,light,true),"Authored armor material unavailable");
    const auto definition=cache.load_initial_model_definition(unit);
    SourceDynamicModelShaders shaders(cache,model,definition.render_model);
    auto vertices=pose_model(model,nullptr,0);
    SourceModelShaderView shader_view;shader_view.shader=source_shader_lod_view(0,480,false,false);
    shader_view.part_shader_multiplier=1;shaders.select(vertices,{10000,255},shader_view);
    // Camera targets the first actual authored emblem part. This is a declared
    // inspection camera, not an original gameplay or customization pose.
    const auto first=std::find_if(vertices.begin(),vertices.end(),[&](const auto &vertex){
      const auto &name=model.mesh.materials.at(vertex.material).name;
      return name.ends_with("\\mc_emblem") || name.ends_with("\\elite_emblem");
    });
    check(first!=vertices.end(),"Actual armor geometry has no selected emblem part");
    // The authored material spans disconnected shoulder/chest patches. A
    // combined normal can point through the armor. Target the largest connected
    // patch, retaining the complete model for depth/placement verification.
    std::vector<unsigned> triangles;
    for(unsigned i=0;i<vertices.size();i+=3)
      if(vertices[i].material==first->material)triangles.push_back(i);
    std::vector<unsigned> patch;
    while(!triangles.empty()) {
      std::vector<unsigned> component{triangles.back()};triangles.pop_back();
      for(unsigned p=0;p<component.size();++p)
        for(auto i=triangles.begin();i!=triangles.end();) {
          bool joined=false;
          for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)
            joined|=length(vertices[component[p]+a].position-vertices[*i+b].position)<1.e-6f;
          if(joined){component.push_back(*i);i=triangles.erase(i);}else ++i;
        }
      if(component.size()>patch.size())patch=std::move(component);
    }
    Vec3 center{},normal{};const unsigned count=unsigned(patch.size())*3;
    for(auto triangle:patch)for(unsigned v=0;v<3;++v) {
      center=center+vertices[triangle+v].position;normal=normal+vertices[triangle+v].normal;
    }
    center=center/float(count);normal=normal/length(normal);
    float radius=0;
    for(auto triangle:patch)for(unsigned v=0;v<3;++v)
      radius=std::max(radius,length(vertices[triangle+v].position-center));
    std::cout<<"Authored emblem material "<<first->material<<" vertices="<<count
        <<" center="<<center.x<<','<<center.y<<','<<center.z
        <<" normal="<<normal.x<<','<<normal.y<<','<<normal.z<<" radius="<<radius<<std::endl;
    check(radius>0 && length(normal)>.9f,"Authored emblem part has no inspection bounds");
    Camera camera;camera.oriented=true;camera.position=center+normal*(radius*3.5f);
    camera.forward=normal*-1.f;camera.up=std::abs(normal.z)>.9f?Vec3{0,1,0}:Vec3{0,0,1};
    camera.field_of_view=60;
    Mesh mesh;mesh.vertices=std::move(vertices);mesh.materials=model.mesh.materials;
    Renderer renderer(window,mesh,true);renderer.scene_clear_color({.025f,.035f,.045f});
    const auto species=representation==2?"spartan":"elite";
    const auto update=[&](unsigned foreground,unsigned flags,unsigned primary,unsigned secondary) {
      SourcePlayerConfiguration profile{};profile[0x45]=foreground;profile[0x47]=flags;
      profile[0x42]=primary;profile[0x43]=secondary;
      for(auto &material:mesh.materials) {
        animate_player_armor_emblem(material,0,profile,1,palette,{});
        apply_object_lighting(material,light);
      }
    };
    update(26,1,9,1);const auto off=capture(renderer,mesh,camera,output/(std::string(species)+"-disabled.ppm"));
    update(26,0,9,1);const auto on=capture(renderer,mesh,camera,output/(std::string(species)+"-page26.ppm"));
    update(5,0,3,4);const auto changed=capture(renderer,mesh,camera,output/(std::string(species)+"-page5.ppm"));
    update(26,0,9,1);const auto repeat=capture(renderer,mesh,camera,output/(std::string(species)+"-repeat.ppm"));
    const auto toggle_pixels=differences(off,on),page_pixels=differences(on,changed);
    std::cout<<species<<": "<<toggle_pixels<<" pixels respond to toggle, "<<page_pixels<<" to selected page/colors\n";
    check(toggle_pixels>50 && page_pixels>50,"Selected emblem did not reach actual armor Vulkan pixels");
    // The original layer2 program uses packed emblem colors directly. Routing
    // it into layer1 also makes it visible, but incorrectly lights those colors.
    const auto primary=pack_object_lighting_rgb(std::bit_cast<Vec3>(palette[9]));
    const auto other_primary=pack_object_lighting_rgb(std::bit_cast<Vec3>(palette[3]));
    const auto exact_pixels=changed_color(off,on,primary);
    check(exact_pixels>50 && changed_color(on,changed,other_primary)>50,
        "Armor emblem was lit or did not retain its original packed palette color");
    std::cout<<species<<": "<<exact_pixels<<" changed pixels retain the exact original primary RGB\n";
    check(differences(on,repeat)==0,"GPU emblem retained another profile's state");
  }
  SDL_DestroyWindow(window);SDL_Quit();
  std::cout<<"Authored armor models/programs/samplers and GPU page/color/toggle delivery pass; original Xbox framebuffer equality remains unmeasured\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
