#include "halo2/cache.hpp"
#include "halo2/appearance.hpp"
#include "halo2/content.hpp"
#include "halo2/model.hpp"
#include "halo2/simulation.hpp"
#include "halo2/renderer.hpp"
#include "halo2/math.hpp"
#include "halo2/transform.hpp"
#include <fstream>
#include <iostream>

namespace {
void check(bool condition,const char *message) {
  if(!condition) throw std::runtime_error(message);
}
template<class T> void write(std::ofstream &s,const T &v) {
  s.write(reinterpret_cast<const char*>(&v),sizeof(v));
}
}
int main(int argc,char **argv) {
  try {
    using namespace halo2;
    if(argc<2) throw FormatError("weapon display test requires extracted maps");
    Cache cache(std::filesystem::path(argv[1])/"01b_spacestation.map");
    auto content=cache.load_content(0);
    const auto weapon=std::find_if(content.weapons.begin(),content.weapons.end(),[](const auto &entry) {
      return entry.second.name.ends_with("\\battle_rifle");
    });
    check(weapon!=content.weapons.end(),"original battle rifle definition missing");
    auto model=cache.load_model(weapon->second.first_person_model);
    auto digit=[&](const char *suffix)->Material & {
      const auto m=std::find_if(model.mesh.materials.begin(),model.mesh.materials.end(),
          [&](const auto &m){return m.name.ends_with(suffix);});
      check(m!=model.mesh.materials.end(),"authored digit surface missing");
      return *m;
    };
    auto &ones=digit("\\ones"), &tens=digit("\\tens");
    const auto &lens=digit("\\battle_rifle_lens");
    check(lens.environment_pass && lens.forward_pass && !lens.static_lightmap_pass,
        "scope lens dropped the base/environment pass sequence");
    const auto &environment=*lens.environment_pass;
    const auto authored=std::find_if(lens.passes.begin(),lens.passes.end(),[](const auto &p) {
      return p.implementation==1 && p.vertex_shader.ends_with("\\lightmap_pervertex_env");
    });
    check(authored!=lens.passes.end() && environment.forward_program.words==authored->program.words &&
        environment.vertex_program==authored->vertex_code && environment.accumulation_texture &&
        environment.forward_raster.depth_function==0x202 && environment.forward_raster.depth_write==0,
        "scope environment shader, accumulation binding or equal-depth state differs from retail");
    const auto &cube=lens.bitmaps[4];
    const auto original_cube=cache.load_bitmap(cube.datum,cube.index);
    check(environment.detail.layers==6 && environment.auxiliary.layers==6 &&
        environment.detail.pixels==original_cube.pixels &&
        environment.detail.mips.size()==original_cube.mips.size(),
        "scope cube faces/mips or authored normalization cube were substituted");
    for(const auto *m:{&ones,&tens}) {
      check(m->forward_pass && m->animated_textures[0].size()==10,
          "all ten authored digit frames must be resident");
      check(m->texture_states[0].mip_filter==2 && m->texture_states[0].address_u==1,
          "digit sampling differs from authored state");
      for(unsigned i=0;i<10;++i) {
        const auto image=cache.load_bitmap(m->bitmaps[0].datum,i);
        check(m->animated_textures[0][i].pixels==image.pixels &&
            image.format==14 && image.mips.size()==4,
            "digit images lost original BC1 data or mip chain");
      }
    }
    std::ofstream output;
    if(argc>2) {output.open(argv[2],std::ios::binary);check(bool(output),"cannot write display vectors");}
    for(int loaded=0;loaded<100;++loaded) {
      auto input=[&](std::uint32_t name){return weapon_ammunition_input(name,loaded).value_or(0.f);};
      animate_material_bitmaps(ones,0,input);animate_material_bitmaps(tens,0,input);
      check(ones.texture_frames[0]==unsigned(loaded%10) && tens.texture_frames[0]==unsigned(loaded/10),
          "magazine count selected the wrong number texture");
      if(output) {
        write(output,loaded);write(output,*weapon_ammunition_input(0x1700056b,loaded));
        write(output,*weapon_ammunition_input(0x1700056c,loaded));
        write(output,ones.texture_frames[0]);write(output,tens.texture_frames[0]);
      }
    }
    check(!weapon_ammunition_input(0xdeadbeef,36),"unknown weapon function must stay unresolved");
    Simulation simulation(content,0,cache.spawns().front());simulation.equip(weapon->first);
    unsigned updates=0;bool emptied=false,refilled=false;
    for(unsigned tick=0;tick<900 && !refilled;++tick) {
      Controls controls;controls.yaw=cache.spawns().front().facing;
      controls.fire=!emptied;controls.reload=emptied;
      simulation.step(controls);
      const auto loaded=simulation.player().weapon.loaded;
      auto input=[&](std::uint32_t name){return weapon_ammunition_input(name,loaded).value_or(0.f);};
      animate_material_bitmaps(ones,tick*tick_seconds,input);
      animate_material_bitmaps(tens,tick*tick_seconds,input);
      check(ones.texture_frames[0]==unsigned(loaded%10) && tens.texture_frames[0]==unsigned(loaded/10),
          "display disagrees with magazine during fire/reload");
      emptied|=loaded==0;refilled=emptied && loaded==weapon->second.capacity;++updates;
    }
    check(emptied && refilled,"fire/reload fixture did not reach empty and refilled states");
    if(argc>3) {
      const auto directory=std::filesystem::path(argv[3]);std::filesystem::create_directories(directory);
      auto mesh=cache.load_bsp(0);
      auto arms=cache.load_model(content.first_person_hands);
      const auto colors=cache.load_object_colors(content.player.datum);
      for(auto &m:arms.mesh.materials) apply_object_colors(m,colors);
      const auto arm_base=unsigned(mesh.materials.size());
      mesh.materials.insert(mesh.materials.end(),arms.mesh.materials.begin(),arms.mesh.materials.end());
      const auto gun_base=unsigned(mesh.materials.size());
      mesh.materials.insert(mesh.materials.end(),model.mesh.materials.begin(),model.mesh.materials.end());
      const auto idle=cache.load_action(weapon->second.first_person_animation,"any","any","any","idle");
      const auto pose=pose_first_person_models(model,arms,idle,0,weapon->second.first_person_offset);
      const auto spawn=cache.spawns().front();
      Camera camera;camera.position=spawn.position+Vec3{0,0,content.player.camera_height};camera.yaw=spawn.facing;
      camera.field_of_view=content.player.camera_field_of_view*(180.f/3.14159265358979323846f);
      camera.horizontal_field_of_view=true; // The authored player FOV is horizontal.
      const auto f=direction(camera.yaw,0),left=Vec3{-std::sin(camera.yaw),std::cos(camera.yaw),0};
      const auto view=camera_transform(camera.position,f,cross(f,left));
      std::vector<Vertex> foreground;
      auto append=[&](std::vector<Vertex> vertices,unsigned base) {
        for(auto &v:vertices) {
          const auto rotate=[&](Vec3 p){return (view.basis[0]*p.x+view.basis[1]*p.y)+view.basis[2]*p.z;};
          v.position=rotate(v.position)+view.position;v.normal=rotate(v.normal);
          v.tangent=rotate(v.tangent);v.binormal=rotate(v.binormal);v.material+=base;
        }
        foreground.insert(foreground.end(),vertices.begin(),vertices.end());
      };
      append(pose.arms,arm_base);append(pose.weapon,gun_base);
      if(SDL_Init(SDL_INIT_VIDEO)!=0) throw std::runtime_error(SDL_GetError());
      SDL_Window *window=SDL_CreateWindow("Weapon display verification",0,0,1920,1080,SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
      if(!window) throw std::runtime_error(SDL_GetError());
      {
        Renderer renderer(window,mesh,true);
        unsigned capture=0;
        for(int loaded:{36,33,30,9,0,36}) {
          for(std::size_t i=0;i<model.mesh.materials.size();++i)
            animate_material_bitmaps(mesh.materials[gun_base+i],0,[&](std::uint32_t name) {
              return weapon_ammunition_input(name,loaded).value_or(0.f);
            });
          renderer.update_material_constants(mesh.materials);
          const auto filename="rounds-"+std::to_string(loaded)+(capture==5?"-restored":"")+".ppm";
          check(renderer.draw(camera,directory/filename,{},false,foreground),
              "weapon display GPU capture did not draw");
          ++capture;
        }
        // Reuse one renderer across window changes: extern 3 must be rebound
        // to the new accumulation image after every swapchain recreation.
        for(const auto size:std::array<std::array<int,2>,4>{{{640,480},{1280,720},{3840,2160},{1920,1080}}}) {
          SDL_SetWindowSize(window,size[0],size[1]);SDL_PumpEvents();renderer.resized();
          const auto filename="resize-"+std::to_string(size[0])+"x"+std::to_string(size[1])+".ppm";
          bool drawn=false;
          for(unsigned attempt=0;attempt<4 && !drawn;++attempt)
            drawn=renderer.draw(camera,directory/filename,{},false,foreground);
          check(drawn,"scope accumulation target failed after resize");
        }
      }
      SDL_DestroyWindow(window);SDL_Quit();
    }
    std::cout<<"Weapon display: 100 digit pairs and "<<updates<<" live firing/reload ticks passed\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
