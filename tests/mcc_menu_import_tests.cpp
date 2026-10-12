#include "halo2/frontend.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/game_pause_menu.hpp"
#include "halo2/ui_dds.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char *message) {if(!value)throw FormatError(message);}
bool same_texture(const Texture &a,const Texture &b) {
  return a.width==b.width && a.height==b.height && a.format==b.format &&
      a.pixels==b.pixels && a.mips.size()==b.mips.size();
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=4)throw FormatError("Xbox maps, MCC menus and MCC HUD required");
    const std::filesystem::path maps=argv[1],pack=argv[2],hud=argv[3];
    if(!std::filesystem::exists(pack/"manifest.json"))return 77;
    const auto vista=hud_menu_asset_directory(pack);
    auto content=Cache(maps/"mainmenu.map").load_frontend();
    content.vista=load_vista_settings(vista/"maps/mainmenu.map");
    const auto sampled=load_frontend_ui_assets(content,pack);
    const auto vista_sampled=load_frontend_ui_assets(*content.vista,pack);
    require(sampled.contains("ui\\global_bitmaps\\framing_center"),"MCC framing omitted");
    const auto &keyboard=sampled.at("ui\\screens\\game_shell\\virtual_keyboard\\button_1");
    require(keyboard.size()==2 && keyboard[0].width==70 && keyboard[1].width==70,
        "Verified MCC normal/selected keyboard pages omitted");
    require(vista_sampled.contains("ui\\screens\\game_shell\\virtual_keyboard\\button_1"),"MCC Vista keyboard omitted");
    Mesh original_mesh,hd_mesh;
    FrontendPainter original(content,maps/"fonts",original_mesh);
    FrontendPainter hd(content,maps/"fonts",hd_mesh,{},sampled,pack/"fonts");
    std::size_t pages{},atlases{},vertices{},draws{},source_only{};
    for(const auto &m:hd_mesh.materials) {
      atlases+=m.name.starts_with("font/") && m.diffuse.width>1024;
      auto selected=sampled.find(m.name);
      if(selected!=sampled.end()) {
        bool found=false;
        for(const auto &p:selected->second)found|=same_texture(m.diffuse,p);
        if(!found) {
          const auto other=vista_sampled.find(m.name);
          if(other!=vista_sampled.end())for(const auto &p:other->second)found|=same_texture(m.diffuse,p);
        }
        require(found,"Painter uploaded a page outside its MCC owner");++pages;
      }
    }
    const auto compare=[&](MenuState &state,int pane) {
      const auto a=original.draw(state,5.f,pane,5000),b=hd.draw(state,5.f,pane,5000);
      require(a.vertices.size()==b.vertices.size(),"MCC menu art changed geometry count or text wrapping");
      for(unsigned i=0;i<a.vertices.size();++i) {
        const auto &x=a.vertices[i],&y=b.vertices[i];
        require(x.position.x==y.position.x && x.position.y==y.position.y && x.position.z==y.position.z &&
            x.color.x==y.color.x && x.color.y==y.color.y && x.color.z==y.color.z,
            "MCC menu samples changed logical positions, clipping or color");
        const auto &am=original_mesh.materials.at(x.material),&bm=hd_mesh.materials.at(y.material);
        if(am.name.starts_with("font/"))require(bm.name.starts_with(am.name+"/"),"MCC selector changed logical font owner");
        else require(am.name==bm.name,"MCC samples changed widget submission owner");
      }
      for(int y=-8;y<=488;y+=16)for(int x=-8;x<=648;x+=16)
        require(original.pointer_hit(float(x),float(y))==hd.pointer_hit(float(x),float(y)),"MCC fonts moved a pointer target");
      vertices+=a.vertices.size();++draws;
    };
    for(const auto &[id,screen]:content.screens) {
      MenuState state(content);if(id!=9)state.open(id);
      for(unsigned p=0;p<screen.panes.size();++p)
        for(unsigned selected=0;selected<std::max(1u,unsigned(state.frame().entries.size()));++selected) {
          state.select(selected);compare(state,int(p));
        }
    }
    for(const auto &[id,screen]:content.vista->screens) {
      // Shell variants and retained result/transition definitions are not
      // standalone native factories. The original-owner draws above cover
      // shared variants; this asset test does not create result/session data.
      if(id==5 || id==38 || id==16 || id==185 || id==209) {++source_only;continue;}
      MenuState state(content);
      try {state.open(vista_menu(id));}
      catch(const FormatError &e) {
        if(std::string_view(e.what())!="unimplemented source Vista settings route")throw;
        std::cout<<"Source-only Vista definition "<<id<<": no standalone native factory\n";
        ++source_only;continue;
      }
      for(unsigned p=0;p<std::max(1u,unsigned(screen.panes.size()));++p)
        for(unsigned selected=0;selected<std::max(1u,unsigned(state.frame().entries.size()));++selected) {
          state.select(selected);compare(state,int(p));
        }
    }
    Mesh pause_mesh;
    GamePauseMenu pause(maps,hud,pause_mesh,false,{},pack);
    require(SDL_InitSubSystem(SDL_INIT_TIMER)==0,"Pause test timer failed");
    SDL_Event escape{};escape.type=SDL_KEYDOWN;escape.key.keysym.sym=SDLK_ESCAPE;
    require(pause.event(escape) && pause.visible(),"Pause source did not open");
    SDL_Delay(1100); // Let the actual pause widget's authored intro complete.
    const auto paused=pause.draw(3840,2160);
    std::size_t pause_glyphs{};
    for(const auto &v:paused.vertices) {
      const auto &material=pause_mesh.materials.at(v.material);
      if(!material.name.starts_with("font/"))continue;
      require(material.diffuse.width>1024,"Actual pause text did not select MCC font raster");
      ++pause_glyphs;
    }
    require(pause_glyphs>0,"Actual pause draw produced no text");
    require(atlases>=10 && pages>100 && draws>100 && vertices>1000,"Incomplete MCC menu exercise");
    std::cout<<"PASS "<<sampled.size()<<" Xbox banks, "<<vista_sampled.size()<<" Vista banks, "
      <<pages<<" uploaded pages, "<<atlases<<" font atlases, "<<draws<<" paired draws, "<<vertices
      <<" vertices; logical positions/wrapping/pointer targets preserved; "<<pause_glyphs<<" pause font vertices selected; "
      <<source_only<<" source-only definitions require separate factories\n";
    return 0;
  } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
