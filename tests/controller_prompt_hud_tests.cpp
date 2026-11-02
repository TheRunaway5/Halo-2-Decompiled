#include "halo2/hud.hpp"
#include "halo2/action_prompt_bindings.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/controller_prompt_artwork.hpp"
#include "mcc_native_prompt_hud_fixture.hpp"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
}
int main(int argc,char **argv)try {
  check(argc==2 || argc==3,"Supply prepared MCC map directory and optional vanilla prompt cache");
  const auto maps=std::filesystem::path(argv[1]);
  if(!std::filesystem::is_regular_file(maps/"zanzibar.map"))return 77;
  Cache cache(maps/"zanzibar.map");
  check(cache.is_mcc(),"Mixed-controller rendering requires the normal prepared MCC owner");
  const auto bipeds=cache.select_tags({.group=tag_groups::biped,.require_address=true});
  check(bipeds.begin()!=bipeds.end(),"Map has no biped HUD");
  auto content=cache.load_hud(bipeds.begin()->datum,null_datum);content.widgets.clear();
  const auto weapons=cache.select_tags({.group=tag_groups::weapon,.require_address=true});
  const auto needler=std::find_if(weapons.begin(),weapons.end(),[](const auto &t){return t.name.ends_with("\\needler");});
  check(needler!=weapons.end(),"Map has no authored Needler prompt");
  Mesh mesh;MultiplayerHud hud(std::move(content),gameplay_font_directory(cache),mesh,4);
  MultiplayerState state;
  for(unsigned i=0;i<4;++i) {
    state.players[i].connected=true;
    state.players[i].action_prompt={{1,5,0xface0001},needler->datum,0};
  }
  PlayerPromptDevices devices;devices.bind(0,41,PromptDevice::Xbox);
  devices.bind(1,73,PromptDevice::PlayStation);devices.bind(2,-1,PromptDevice::KeyboardMouse);
  devices.bind(3,109,PromptDevice::PlayStation);
  unsigned draws{};
  const auto draw=[&](unsigned view,unsigned players,unsigned width,unsigned height) {
    hud.prompt_device(view,devices.device(view));
    check(!hud.action_text(state,view,view).empty(),"Authored action prompt disappeared");
    auto vertices=hud.draw(state,view,view,players,width,height,0,0);
    check(!vertices.empty(),"Player's prompt emitted no geometry");
    unsigned controls{};
    for(const auto &v:vertices) {
      const auto &name=mesh.materials.at(v.material).name;
      if(!name.starts_with("controller-prompts/"))continue;
      ++controls;
      check(name.starts_with(devices.device(view)==PromptDevice::PlayStation?
          "controller-prompts/playstation/":"controller-prompts/xbox/"),"A player drew another controller family's artwork");
    }
    check(controls>=6,
          "Device selection failed to reach the actual action HUD icon quads");
    ++draws;return vertices;
  };
  for(auto size:{std::pair{640u,480u},std::pair{1280u,720u},std::pair{3840u,2160u}})
    for(unsigned view=0;view<4;++view)draw(view,4,size.first,size.second);
  auto prior=draw(1,4,1280,720);
  SDL_Event key{};key.type=SDL_KEYDOWN;key.key.windowID=7;devices.event(key,7,0);
  draw(0,4,1280,720);const auto unchanged=draw(1,4,1280,720);
  check(prior.size()==unchanged.size(),"KBM activity changed the other player's prompt length");
  for(unsigned i=0;i<prior.size();++i)check(prior[i].material==unchanged[i].material &&
      prior[i].uv.x==unchanged[i].uv.x && prior[i].uv.y==unchanged[i].uv.y &&
      prior[i].position.x==unchanged[i].position.x && prior[i].position.y==unchanged[i].position.y,
      "A player's keyboard event changed another viewport's emitted prompt geometry");
  devices.bind(0,41,PromptDevice::Xbox); // An unchanged hotplug refresh preserves KBM.
  draw(0,1,640,480);
  SDL_Event button{};button.type=SDL_CONTROLLERBUTTONDOWN;button.cbutton.which=41;devices.event(button,7,0);
  draw(0,1,1280,720);
  devices.bind(0,211,PromptDevice::PlayStation);draw(0,1,1280,720);
  // Clipping uses the same authored rectangle for both families without
  // changing the logical font or ordinary glyph samples.
  ControllerPromptArtwork artwork(mesh,gameplay_font_directory(cache));artwork.prepare(6);
  const auto font=load_gameplay_font_source(gameplay_font_directory(cache));
  unsigned owned_bindings{};
  for(unsigned selector=0;selector<font.names.size();++selector) {
    artwork.prepare(selector);
    const auto raw=load_retail_font(font.directory/font.names.at(selector));
    std::map<std::uint32_t,unsigned> counts;
    for(const auto record:raw.glyph_record_indices)++counts[record];
    for(unsigned code=0xe100;code<=0xe13c;++code) {
      if((code>0xe111 && code<0xe12c) || (code>=0xe136 && code<=0xe13b))continue;
      const auto record=raw.glyph_record_indices.at(code);
      const auto found=raw.glyphs.find(code);
      if(record==0xffffffffu || counts.at(record)!=1 || found==raw.glyphs.end() ||
          !found->second.width || !found->second.height)continue;
      const auto& g=found->second;
      for(const auto device:{PromptDevice::KeyboardMouse,PromptDevice::Xbox,PromptDevice::PlayStation}) {
        const auto sample=artwork.sample(selector,code,device,g,0,0,g.width,g.height);
        check(sample && mesh.materials.at(sample->material).name.starts_with("controller-prompts/"),
            "An active control/keycap glyph sampled MCC button artwork");
      }
      ++owned_bindings;
    }
  }
  check(owned_bindings==331,"Owned button prompt coverage differs from the actual MCC selector bindings");
  const auto actual=load_gameplay_font(font,6);const auto &glyph=actual.glyphs.at(0xe100);
  const auto full=artwork.sample(6,0xe100,PromptDevice::PlayStation,glyph,0,0,glyph.width,glyph.height);
  const auto clipped=artwork.sample(6,0xe100,PromptDevice::PlayStation,glyph,2,3,glyph.width-4,glyph.height-6);
  check(full && clipped && clipped->material==full->material && clipped->uv[0]>full->uv[0] &&
      clipped->uv[1]>full->uv[1] && clipped->uv[2]<full->uv[2] && clipped->uv[3]<full->uv[3],
      "Clipped PlayStation prompt lost its actual atlas rectangle");
  const auto control_font=load_retail_font(font.directory/font.names.at(6));
  for(const auto code:{0xe132u,0xe133u,0xe134u,0xe135u,0xe13cu}) {
    const auto& frame=control_font.glyphs.at(code);
    for(const auto device:{PromptDevice::KeyboardMouse,PromptDevice::Xbox,PromptDevice::PlayStation}) {
      const auto sample=artwork.sample(6,code,device,frame,0,0,frame.width,frame.height);
      check(sample && mesh.materials.at(sample->material).name.starts_with("controller-prompts/"),
          "A keycap/shared prompt fell back to MCC controller glyph artwork");
    }
  }
  {
    const auto cwd=std::filesystem::current_path();
    const auto* layout=std::getenv("H2_INSTALLATION_LAYOUT");
    const bool had_layout=layout!=nullptr;const std::string saved=layout?layout:"";
    const auto restore=[&]{
      std::filesystem::current_path(cwd);
      if(had_layout)SDL_setenv("H2_INSTALLATION_LAYOUT",saved.c_str(),1);
    };
#ifdef _WIN32
    _putenv_s("H2_INSTALLATION_LAYOUT","");
#else
    unsetenv("H2_INSTALLATION_LAYOUT");
#endif
    bool rejected=false;
    try {
      std::filesystem::current_path(cwd.root_path());
      Mesh empty;
      ControllerPromptArtwork missing(empty,cwd.root_path()/"__missing_owned_prompts__/halo2/h2_fonts");
      missing.prepare(6);
      try{(void)missing.sample(6,0xe100,PromptDevice::Xbox,glyph,0,0,glyph.width,glyph.height);}
      catch(const FormatError&){rejected=true;}
    }catch(...){restore();throw;}
    restore();
    check(rejected,"Missing owned Xbox prompts silently fell back to MCC glyph pixels");
  }
  mcc_native_prompt_test::verify_native_mcc_prompt_hud(gameplay_font_directory(cache));
  if(argc==3) {
    const auto *prior_layout=std::getenv("H2_INSTALLATION_LAYOUT");
    const std::string prior=prior_layout?prior_layout:"";
    // The installed vanilla bank must win even if the portable setup source
    // is unavailable. This does not change or copy the map/font owners.
    SDL_setenv("H2_INSTALLATION_LAYOUT",
      "{\"schema\":\"H2_INSTALLATION_LAYOUT_V1\",\"resources\":\"/unavailable-prompt-resources\"}",1);
    ControllerPromptArtwork installed(mesh,std::filesystem::path(argv[2])/"halo2/h2_fonts");
    installed.prepare(6);
    check(bool(installed.sample(6,0xe100,PromptDevice::PlayStation,glyph,0,0,glyph.width,glyph.height)),
        "Vanilla onboarding cache did not provide its installed PlayStation artwork");
    SDL_setenv("H2_INSTALLATION_LAYOUT",prior.c_str(),1);
  }
  std::cout<<"PASS mixed controller HUD: players=4 draws="<<draws<<" sizes=3 families=Xbox,PlayStation,KBM isolation=exact owned_bindings="<<owned_bindings<<"\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
