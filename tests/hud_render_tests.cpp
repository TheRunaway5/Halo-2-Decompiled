#include "halo2/content.hpp"
#include "halo2/hud.hpp"
#include "halo2/renderer.hpp"
#include "halo2/original_data.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/texture.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void check(bool condition,const std::string &message) {
  if(!condition)throw std::runtime_error(message);
}
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum;
  Image image;file>>magic>>image.width>>image.height>>maximum;file.get();
  check(magic=="P6" && maximum==255,"Invalid HUD framebuffer capture");
  image.pixels.resize(image.width*image.height*3);
  file.read(reinterpret_cast<char*>(image.pixels.data()),image.pixels.size());
  check(bool(file),"Incomplete HUD framebuffer capture");return image;
}
void shield_top_seam(Cache &cache,const std::filesystem::path &fonts,
                     const std::filesystem::path &directory,SDL_Window *window) {
  if(!cache.is_mcc())return;
  const auto *globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
  check(globals && globals->address,"Missing shield representation owner");
  const Bytes representations(cache.read_tag_bytes(globals->address+0x138,8));
  unsigned columns=0;
  for(unsigned representation:{2u,3u}) {
    const Bytes biped(cache.read_tag_bytes(representations.u32(4)+representation*0xbc+0xb4,4));
    auto content=cache.load_hud(biped.u32(0),cache.slayer_starting_weapon());
    std::erase_if(content.widgets,[](const auto &w){return w.name!="shield_meter" && w.name!="shield_mask";});
    check(content.widgets.size()==2,"Missing actual shield fill/outline pair");
    Mesh mesh;mesh.vertices.resize(3);MultiplayerHud hud(std::move(content),fonts,mesh,1);
    Renderer renderer(window,mesh,true);renderer.scene_clear_color({0,0,0});
    MultiplayerState state;auto &player=state.players[0];player.connected=true;player.shield=player.health=1;
    if(representation==3){player.source_configuration=SourcePlayerConfiguration{};(*player.source_configuration)[0x44]=1;}
    for(const auto dimensions:std::array<std::array<int,2>,3>{{{640,480},{1280,720},{3840,2160}}}) {
      SDL_SetWindowSize(window,dimensions[0],dimensions[1]);SDL_PumpEvents();renderer.resized();
      const auto suffix=std::to_string(representation)+"-"+std::to_string(dimensions[0]);
      for(float amount:{1.f,2.5f}) {
        ++state.tick;player.shield=amount;hud.animate(state,0,0,25,14400,state.tick/30.0);
        const auto ui=hud.draw(state,0,0,1,dimensions[0],dimensions[1],25,14400);
        renderer.update_material_constants(mesh.materials);
        const auto path=directory/("shield-seam-"+suffix+"-"+std::to_string(amount)+".ppm");
        bool drawn=false;for(unsigned retry=0;retry<4 && !drawn;++retry)
          drawn=renderer.draw(Camera{},path,{},false,{},{},ui);
        check(drawn,"Shield seam framebuffer was not rendered");const auto image=read(path);
        check(image.width==unsigned(dimensions[0]) && image.height==unsigned(dimensions[1]),
              "Shield seam capture has the wrong drawable dimensions");
        const auto outline=std::find_if(ui.begin(),ui.end(),[&](const auto &v){
          return !mesh.materials.at(v.material).name.ends_with("shield_test");
        });
        check(outline!=ui.end(),"Shield seam lacks its outline");
        const float left=outline->position.x*image.width/640,
            top=outline->position.y*image.height/480,
            width=(outline[1].position.x-outline->position.x)*image.width/640;
        const float scale=float(image.height)/480;
        for(unsigned x=unsigned(left+width*.4f);x<unsigned(left+width*.6f);++x) {
          bool edge=false;unsigned empty=0;
          for(unsigned y=unsigned(std::ceil(top));y<unsigned(top+8*scale);++y) {
            const auto at=(y*image.width+x)*3;
            // The reported seam is the dark outline interior (RGB17/26/36
            // in the supplied capture), not necessarily zero-alpha padding.
            const bool lit=std::max({image.pixels.at(at),image.pixels.at(at+1),image.pixels.at(at+2)})>64;
            if(lit) {
              check(!edge || empty==0,"Shield fill has a dark gap below its top outline: "+suffix+
                    " pixel "+std::to_string(x)+","+std::to_string(y)+" empty rows="+std::to_string(empty));
              edge=true;
            } else if(edge)++empty;
          }
          check(edge,"Shield top outline was not exercised");++columns;
        }
      }
    }
  }
  check(columns>100,"Shield seam coverage is incomplete");
  std::cout<<columns<<" full-shield/overshield columns have no top seam, both species at SD, HD and 4K\n";
}
}
int main(int argc,char **argv) {
  try {
    check(argc==3 || argc==4,"HUD render test requires maps, capture directory and optional UI assets");
#if defined(__linux__)
    if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY")) {
      std::cout<<"HUD framebuffer test needs an SDL display\n";return 77;
    }
#endif
    const auto maps=std::filesystem::path(argv[1]),directory=std::filesystem::path(argv[2]);
    std::filesystem::create_directories(directory);
    Cache cache(maps/"lockout.map",maps,OriginalData::native_tu15());
    const auto fonts=gameplay_font_directory(cache);
    auto content=cache.load_hud(cache.load_content(0,true).player.datum,cache.slayer_starting_weapon());
    std::erase_if(content.widgets,[](const auto &w){return w.name!="shield_meter";});
    check(content.widgets.size()==1,"Missing original shield meter");
    Mesh mesh;MultiplayerHud hud(std::move(content),fonts,mesh,4,"Slayer",{},
                                argc==4?std::filesystem::path(argv[3]):std::filesystem::path{});
    // Keep the world buffer valid without covering the known backdrop.
    mesh.vertices.resize(3);
    MultiplayerState state;for(auto &player:state.players) {
      player.connected=true;player.shield=player.health=1;
    }
    // Alternating actual appearance bytes exercise the independent shield
    // enable flag and the source's second sprite selection in each aspect.
    for(unsigned id:{1u,3u}) {
      state.players[id].source_configuration=SourcePlayerConfiguration{};
      (*state.players[id].source_configuration)[0x44]=1;
    }
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
    auto *window=SDL_CreateWindow("Authored HUD alpha-kill verification",0,0,640,480,
                                  SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
    check(window,SDL_GetError());unsigned checked=0,changed=0;
    if(argc==3)shield_top_seam(cache,fonts,directory,window);
    {
      Renderer renderer(window,mesh,true);renderer.scene_clear_color({.4f,.6f,.8f});
      Camera camera;
      auto capture=[&](const std::string &name,std::span<const Vertex> ui,
                       std::span<const ModelView> views) {
        const auto path=directory/(name+".ppm");bool drawn=false;
        for(unsigned retry=0;retry<4 && !drawn;++retry)
          drawn=renderer.draw(camera,path,{},false,{}, {},ui,views);
        check(drawn,"HUD framebuffer was not rendered");return read(path);
      };
      for(const auto dimensions:std::array<std::array<int,2>,2>{{{640,480},{1280,720}}}) {
        SDL_SetWindowSize(window,dimensions[0],dimensions[1]);SDL_PumpEvents();renderer.resized();
        const auto suffix=std::to_string(dimensions[0])+"x"+std::to_string(dimensions[1]);
        std::array<ModelView,4> views;
        for(unsigned view=0;view<4;++view) {
          const auto rectangle=hud_viewport(4,view,dimensions[0]*3>dimensions[1]*4).outer;
          views[view].camera=camera;views[view].left=rectangle.left;views[view].top=rectangle.top;
          views[view].width=rectangle.right-rectangle.left;
          views[view].height=rectangle.bottom-rectangle.top;views[view].full_surface=true;
        }
        const auto baseline=capture("backdrop-"+suffix,{},views);
        for(const float shield:{1.f,.25f,.75f,2.5f}) {
          ++state.tick;for(auto &player:state.players)player.shield=shield;
          std::vector<Vertex> ui;
          for(unsigned view=0;view<4;++view) {
            views[view].before_ui_vertex=ui.size();
            hud.animate(state,view,view,25,14400,state.tick/30.0);
            auto vertices=hud.draw(state,view,view,4,baseline.width,baseline.height,25,14400);
            ui.insert(ui.end(),vertices.begin(),vertices.end());
          }
          renderer.update_material_constants(mesh.materials);
          const auto image=capture("shield-"+suffix+"-"+std::to_string(shield),ui,views);
          for(unsigned quad=0;quad<ui.size();quad+=6) {
            const auto &a=ui[quad],&b=ui[quad+1],&c=ui[quad+2];
            const auto &m=mesh.materials[a.material];
            const auto texture=m.diffuse.format>=14 && m.diffuse.format<=16?
                prepare_xemu_bc_texture(m.diffuse):m.diffuse;
            check((m.texture_states[0].alpha_kill&4)!=0 && m.forward_blend==4,
                  "Original shield alpha-kill or ONE/SRC_ALPHA blend changed");
            const float left=a.position.x*image.width/640,right=b.position.x*image.width/640,
                        top=a.position.y*image.height/480,bottom=c.position.y*image.height/480;
            for(unsigned y=unsigned(std::ceil(top));y<unsigned(std::floor(bottom));++y)
            for(unsigned x=unsigned(std::ceil(left));x<unsigned(std::floor(right));++x) {
              // Require all four bilinear source taps to have zero alpha.
              // Retail alpha-kill acts on the fetched texel before combiners,
              // so these fragments must preserve every destination channel.
              const int tx=int(std::floor((a.uv.x+((x+.5f-left)/(right-left))*(b.uv.x-a.uv.x))*texture.width-.5f)),
                        ty=int(std::floor((a.uv.y+((y+.5f-top)/(bottom-top))*(c.uv.y-a.uv.y))*texture.height-.5f));
              if(tx<0 || ty<0 || tx+1>=int(texture.width) || ty+1>=int(texture.height))continue;
              bool transparent=true;
              for(int dy=0;dy<2;++dy)for(int dx=0;dx<2;++dx)
                transparent&=texture.pixels[((ty+dy)*texture.width+tx+dx)*4+3]==std::byte{};
              const auto at=(y*image.width+x)*3;
              bool equal=true;for(unsigned channel=0;channel<3;++channel)
                equal&=image.pixels[at+channel]==baseline.pixels[at+channel];
              if(transparent) {
                check(equal,"Transparent shield padding darkens framebuffer at "+suffix+
                            " pixel "+std::to_string(x)+","+std::to_string(y));++checked;
              }else changed+=!equal;
            }
          }
        }
      }
    }
    {
      const auto *globals=cache.find_tag(cache.globals_tag(),tag_groups::globals);
      check(globals && globals->address,"Missing shield/radar representation owner");
      const Bytes representations(cache.read_tag_bytes(globals->address+0x138,8));
      check(representations.u32(0)>=4,"Missing Spartan/Elite HUD rows");
      Mesh radar_mesh;radar_mesh.vertices.resize(3);
      std::map<unsigned,std::unique_ptr<MultiplayerHud>> huds;
      for(unsigned representation:{2u,3u}) {
        const Bytes biped(cache.read_tag_bytes(representations.u32(4)+representation*0xbc+0xb4,4));
        auto content=cache.load_hud(biped.u32(0),
                                    cache.slayer_starting_weapon());
        std::erase_if(content.widgets,[](const auto &w){return w.name!="motion_tracker_background" &&
            w.name!="shield_meter" && w.name!="shield_mask";});
        huds.emplace(representation,std::make_unique<MultiplayerHud>(std::move(content),fonts,
            radar_mesh,4,"Slayer",std::nullopt,argc==4?std::filesystem::path(argv[3]):std::filesystem::path{}));
      }
      Renderer renderer(window,radar_mesh,true);renderer.scene_clear_color({.4f,.6f,.8f});
      MultiplayerState radar_state;
      for(unsigned n=0;n<4;++n) {
        auto &p=radar_state.players[n];p.connected=true;p.shield=p.health=1;
        p.source_player=0x10000+n;p.source_unit=0x20000+n;p.source_membership=n;
        p.position={float(n)*2,0,0};
        if(n%2==0){p.source_configuration=SourcePlayerConfiguration{};(*p.source_configuration)[0x44]=1;}
      }
      std::vector<UiRect> sensor_quads;
      auto capture=[&](const std::string &name,unsigned players,unsigned tick,bool moving,bool overlay_only=false,
                       std::uint8_t trigger=0,std::uint8_t grenade_phase=0) {
        // Both captures use the same final source tick and the real imported
        //artwork. Only the presence of a moving nearby participant changes.
        for(unsigned t=tick-9;t<=tick;++t) {
          radar_state.tick=t;radar_state.players[1].velocity=moving?Vec3{2,0,0}:Vec3{};
          radar_state.players[1].motion_sensor_control=trigger;
          radar_state.players[1].grenade_action.source[0xc]=std::byte{grenade_phase};
          for(unsigned view=0;view<players;++view) {
            auto &hud=*huds.at(view%2?2:3);hud.update_motion_sensor(radar_state,view,view);
          }
        }
        int width,height;SDL_Vulkan_GetDrawableSize(window,&width,&height);
        std::array<ModelView,4> views;std::vector<Vertex> ui;sensor_quads.clear();
        for(unsigned view=0;view<players;++view) {
          const auto rectangle=hud_viewport(players,view,width*3>height*4).outer;
          auto &v=views[view];v.camera=Camera{};v.left=rectangle.left;v.top=rectangle.top;
          v.width=rectangle.right-rectangle.left;v.height=rectangle.bottom-rectangle.top;v.full_surface=true;
          v.before_ui_vertex=ui.size();auto &hud=*huds.at(view%2?2:3);
          hud.animate(radar_state,view,view,25,14400,tick/30.0);
          const auto material=*hud.motion_sensor_material(view);
          renderer.update_image(material,radar_mesh.materials.at(material).diffuse);
          const auto vertices=hud.draw(radar_state,view,view,players,width,height,25,14400);
          for(unsigned q=0;q<vertices.size();q+=6) {
            if(overlay_only && vertices[q].material!=material)continue;
            ui.insert(ui.end(),vertices.begin()+q,vertices.begin()+q+6);
            if(vertices[q].material==material) {
              UiRect rectangle{vertices[q].position.y*height/480,vertices[q].position.x*width/640,
                  vertices[q+2].position.y*height/480,vertices[q+1].position.x*width/640};
              sensor_quads.push_back(rectangle);
            }
          }
        }
        renderer.update_material_constants(radar_mesh.materials);
        const auto path=directory/(name+".ppm");bool drawn=false;
        for(unsigned retry=0;retry<4 && !drawn;++retry)
          drawn=renderer.draw(Camera{},path,{},false,{}, {},ui,std::span(views).first(players));
        check(drawn,"Live radar framebuffer did not render");return read(path);
      };
      unsigned contact_pixels=0,pulse_pixels=0,preserved_corners=0;
      for(const auto dimensions:std::array<std::array<int,2>,2>{{{640,480},{1280,720}}}) {
        SDL_SetWindowSize(window,dimensions[0],dimensions[1]);SDL_PumpEvents();renderer.resized();
        for(unsigned players=1;players<=4;++players) {
          const auto suffix=std::to_string(dimensions[0])+"x"+std::to_string(dimensions[1])+"-"+std::to_string(players);
          const auto idle=capture("radar-idle-"+suffix,players,40,false);
          const auto moving=capture("radar-contacts-"+suffix,players,40,true);
          const auto contact_bounds=sensor_quads;
          const auto offhand=capture("radar-offhand-"+suffix,players,40,false,false,0x20);
          const auto throwing=capture("radar-throwing-"+suffix,players,40,false,false,0,2);
          const auto released=capture("radar-released-"+suffix,players,40,false,false,0,3);
          check(offhand.pixels==moving.pixels && throwing.pixels==moving.pixels && released.pixels==idle.pixels,
              "Vulkan radar did not retain the original offhand/throw-phase visibility gates");
          // Tick40 and62 both place the authored luminous ring beyond the
          // mask. Compare an early visible ring phase against the idle tail.
          const auto pulsed=capture("radar-pulse-"+suffix,players,80,false);
          unsigned contacts=0,pulses=0;
          for(unsigned at=0;at<idle.pixels.size();at+=3) {
            const bool contact=!std::equal(idle.pixels.begin()+at,idle.pixels.begin()+at+3,moving.pixels.begin()+at);
            if(contact) {
              const float x=(at/3)%idle.width+.5f,y=(at/3)/idle.width+.5f;
              check(std::any_of(contact_bounds.begin(),contact_bounds.end(),[&](const auto &r){
                return x>=r.left && x<r.right && y>=r.top && y<r.bottom;
              }),"A local radar contact changed pixels outside every sensor quad");
            }
            contacts+=contact;
            pulses+=!std::equal(idle.pixels.begin()+at,idle.pixels.begin()+at+3,pulsed.pixels.begin()+at);
          }
          check(contacts>10 && pulses>10,"Live Vulkan radar does not show contacts and sweep changes");
          contact_pixels+=contacts;pulse_pixels+=pulses;
        }
        const auto suffix=std::to_string(dimensions[0])+"x"+std::to_string(dimensions[1]);
        const auto backdrop=capture("radar-overlay-backdrop-"+suffix,0,63,false,true);
        // Exercise a whole source sweep on the actual Vulkan final shader,
        // with both species and four independent cells. The radar overlay
        // may add light; its RGB-only intermediate must never dim a backdrop.
        for(unsigned tick=63;tick<126;++tick) {
          const auto overlay=capture("radar-overlay-"+suffix+"-"+std::to_string(tick),4,tick,false,true);
          for(unsigned at=0;at<overlay.pixels.size();++at)
            check(overlay.pixels[at]>=backdrop.pixels[at],"Radar pulse darkened Vulkan framebuffer");
          for(const auto &rectangle:sensor_quads)
            for(float y:{rectangle.top+1,rectangle.bottom-2})for(float x:{rectangle.left+1,rectangle.right-2}) {
              const auto at=(unsigned(y)*overlay.width+unsigned(x))*3;
              for(unsigned c=0;c<3;++c)
                check(overlay.pixels.at(at+c)==backdrop.pixels.at(at+c),"Radar square corner appears during Vulkan pulse");
              ++preserved_corners;
            }
        }
      }
      std::cout<<contact_pixels<<" visible Vulkan contact pixels and "<<pulse_pixels
               <<" pulse changes across1-4 views and both aspects; actual species shield outlines rendered\n";
      std::cout<<preserved_corners<<" unchanged square corners and no backdrop darkening through complete Vulkan sweep cycles\n";
    }
    SDL_DestroyWindow(window);SDL_Quit();
    check(checked>1000 && changed>100,"HUD regression did not exercise padding and visible shield fill");
    std::cout<<checked<<" transparent shield padding samples preserved; "<<changed
             <<" visible fill samples rendered across four views, damage, recharge and overshields\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';SDL_Quit();return 1;}
}
