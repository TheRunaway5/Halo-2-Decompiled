#define SDL_MAIN_HANDLED
#include "halo2/renderer.hpp"
#include "halo2/world_material.hpp"
#include "halo2/model.hpp"
#include "halo2/appearance.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
struct Frame{unsigned width{},height{};std::vector<unsigned char> pixels;};
Frame read(const std::filesystem::path &path) {
  std::ifstream f(path,std::ios::binary);Frame result;std::string magic;unsigned maximum;
  f>>magic>>result.width>>result.height>>maximum;f.get();
  check(magic=="P6" && maximum==255,"Invalid camouflage capture");
  result.pixels.resize(std::size_t(result.width)*result.height*3);
  f.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());check(bool(f),"Truncated camouflage capture");
  return result;
}
unsigned difference(const Frame&a,const Frame&b,unsigned owner) {
  check(a.width==b.width && a.height==b.height,"Camouflage capture extent changed");unsigned changed=0;
  for(unsigned y=0;y<a.height;++y)for(unsigned x=owner*a.width/2;x<(owner+1)*a.width/2;++x)
    for(unsigned c=0;c<3;++c){const auto i=(std::size_t(y)*a.width+x)*3+c;
      changed+=std::abs(int(a.pixels[i])-int(b.pixels[i]))>1;}
  return changed;
}
}
int main(int argc,char **argv)try {
  check(argc==3,"Map directory and capture directory required");
  if(!std::getenv("DISPLAY"))return 77;
  Cache cache(std::filesystem::path(argv[1])/"lockout.map");const auto content=cache.load_content(0,true);
  const auto tag=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag){
    return tag.group=="weap" && tag.name=="objects\\weapons\\rifle\\covenant_carbine\\covenant_carbine";});
  check(tag!=cache.tags().end(),"Original carbine fixture missing");
  const auto &weapon=content.weapons.at(tag->datum);const auto gun=cache.load_model(weapon.first_person_model);
  const auto display=std::find_if(gun.mesh.materials.begin(),gun.mesh.materials.end(),[](const auto &m) {
    return m.name=="objects\\weapons\\rifle\\covenant_carbine\\shaders\\carbine_display";
  });
  check(display!=gun.mesh.materials.end(),"Original carbine display material missing");
  auto material=*display;
  check(prepare_dynamic_material(cache,material,{},true,0.f),"Original carbine display program unresolved");
  const auto &pass=material.passes.at(material.forward_pass_index);
  check(std::any_of(pass.pixel_externs.begin(),pass.pixel_externs.end(),[](const auto &r) {
    return r.source==37 && (r.component&7)<4;
  }) && !material.animation.overlays.empty(),"Authored animated camouflage fixture changed");
  apply_object_colors(material,cache.load_object_colors(weapon.datum));
  const bool aliases=std::getenv("HALO2_TEST_CAMOUFLAGE_OBJECT_ALIASES")!=nullptr;
  Mesh mesh;mesh.materials=aliases?std::vector<Material>{material}:std::vector<Material>{material,material};const unsigned bank=1;
  Camera camera;
  // Declared front-facing test surfaces isolate the actual compiled display
  // programs, textures, raster state and animation. This is GPU uniform
  // delivery evidence, not a claim about the full gun pose or camo clone.
  std::array<std::vector<Vertex>,2> vertices;
  for(unsigned owner=0;owner<2;++owner)
    for(const auto uv:std::array{Vec2{0,0},Vec2{1,0},Vec2{1,1},Vec2{0,0},Vec2{1,1},Vec2{0,1}}) {
      Vertex v;v.position={2,(uv.x*2-1)*.5f,(1-uv.y*2)*.5f};v.uv=uv;
      v.color={1,1,1};v.normal={-1,0,0};v.tangent={0,1,0};v.binormal={0,0,-1};
      v.material=aliases?0:owner;vertices[owner].push_back(v);
    }
  SDL_SetMainReady();check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
  std::unique_ptr<SDL_Window,decltype(&SDL_DestroyWindow)> window(SDL_CreateWindow("Original camouflage material owners",0,0,1280,480,
      SDL_WINDOW_VULKAN|SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN),&SDL_DestroyWindow);check(bool(window),SDL_GetError());
  const std::filesystem::path output=argv[2];std::filesystem::create_directories(output);
  {
    Renderer renderer(window.get(),mesh,!aliases);
    for(const auto size:std::array<std::array<int,2>,2>{{{1280,480},{1920,720}}}) {
      SDL_SetWindowSize(window.get(),size[0],size[1]);SDL_PumpEvents();renderer.resized();
      std::array<ModelView,2> views;
      std::array<std::array<ModelObjectDraw,1>,2> objects;
      for(unsigned i=0;i<2;++i){auto &v=views[i];v.camera=camera;v.left=float(i*320);v.width=320;v.height=480;
        v.full_surface=true;
        if(aliases) {objects[i][0].vertices=vertices[i];objects[i][0].shader_input=[](auto){return 1.f;};v.objects=objects[i];}
        else v.foreground=vertices[i];}
      const auto capture=[&](float a,float b,const char *label){
        if(aliases)for(unsigned i=0;i<2;++i)objects[i][0].camouflage=i?b:a;
        else for(unsigned i=0;i<mesh.materials.size();++i) {
          auto &material=mesh.materials[i];bind_object_camouflage(material,i<bank?a:b);
          const auto animate=[&](auto&self,Material&m)->void {
            if(m.forward_pass)animate_forward_material(m,0,[](auto){return 1.f;});
            if(m.environment_pass)self(self,*m.environment_pass);
            for(auto&p:m.world_overlays_before)self(self,*p);for(auto&p:m.world_overlays_after)self(self,*p);
            for(auto&p:m.world_bloom_passes)self(self,*p);
          };animate(animate,material);
        }
        renderer.update_material_constants(mesh.materials);
        const auto path=output/(std::to_string(size[0])+"-"+label+".ppm");
        for(unsigned attempt=0;attempt<5;++attempt)if(renderer.draw(camera,path,{},false,{},{},{},views))return read(path);
        throw FormatError("Camouflage capture did not render");
      };
      const auto mixed=capture(0,1,"mixed"),zero=capture(0,0,"zero"),one=capture(1,1,"one"),swapped=capture(1,0,"swapped");
      check(!difference(mixed,zero,0) && !difference(mixed,one,1),"Camouflage leaked between material owners");
      check(!difference(swapped,one,0) && !difference(swapped,zero,1),"Changed camouflage retained another owner's bank");
      const auto left=difference(zero,one,0),right=difference(zero,one,1);
      if(left<=10 || right<=10)throw FormatError("Authored camouflage scalar produced no measured GPU response: "+std::to_string(left)+"/"+std::to_string(right));
      const auto reset=capture(0,1,"reset");
      check(!difference(mixed,reset,0) && !difference(mixed,reset,1),"Camouflage retained a stale prior frame");
      std::cout<<size[0]<<'x'<<size[1]<<": independent authored camouflage response "<<left<<'/'<<right<<" components\n";
    }
  }
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
