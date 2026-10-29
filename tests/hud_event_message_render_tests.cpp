#include "halo2/cache.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/hud.hpp"
#include "halo2/original_data.hpp"
#include "halo2/renderer.hpp"
#include "halo2/source_event_messages_tu15.hpp"
#include <SDL_vulkan.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char* message) {if(!value)throw FormatError(message);}
bool near(float a,float b) {return std::abs(a-b)<.001f;}
struct Files {
  std::filesystem::path path;
  Files() {
    const auto* root=std::getenv("HALO2_TEST_SCRATCH");
    check(root && *root,"Event render test requires managed scratch");
    for(unsigned n=0;;++n) {
      path=std::filesystem::path(root)/("event-message-render-"+std::to_string(n));
      if(std::filesystem::create_directory(path))break;
    }
  }
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
struct Image {unsigned width{},height{};std::vector<unsigned char> pixels;};
Image read(const std::filesystem::path& path) {
  std::ifstream file(path,std::ios::binary);std::string magic;unsigned maximum{};
  Image result;file>>magic>>result.width>>result.height>>maximum;file.get();
  check(magic=="P6" && maximum==255,"Invalid event-message framebuffer");
  result.pixels.resize(std::size_t(result.width)*result.height*3);
  file.read(reinterpret_cast<char*>(result.pixels.data()),result.pixels.size());
  check(bool(file),"Incomplete event-message framebuffer");return result;
}
unsigned changed(const Image& baseline,const Image& image,UiRect rectangle) {
  check(baseline.width==image.width && baseline.height==image.height,"Event framebuffer dimensions changed");
  unsigned result{};
  for(unsigned y=unsigned(rectangle.top*image.height/480);y<unsigned(rectangle.bottom*image.height/480);++y)
  for(unsigned x=unsigned(rectangle.left*image.width/640);x<unsigned(rectangle.right*image.width/640);++x) {
    const auto at=(std::size_t(y)*image.width+x)*3;
    result+=!std::equal(image.pixels.begin()+at,image.pixels.begin()+at+3,baseline.pixels.begin()+at);
  }
  return result;
}
void gpu(Mesh& mesh,MultiplayerHud& hud,MultiplayerState state,
    const FullScoreboardPaintConstants& constants) {
#if defined(__linux__)
  if(!std::getenv("DISPLAY") && !std::getenv("WAYLAND_DISPLAY"))
    throw FormatError("Event framebuffer test requires an SDL display");
#endif
  Files files;check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());
  auto* window=SDL_CreateWindow("Native event-message framebuffer",0,0,1280,720,
      SDL_WINDOW_VULKAN|SDL_WINDOW_HIDDEN);
  check(window,SDL_GetError());
  try {
    mesh.vertices.resize(3);
    {
      Renderer renderer(window,mesh,true);renderer.scene_clear_color({.05f,.08f,.1f});
      // Use the same four full-surface world viewports for the empty baseline
      // and text draws. The menu-only empty/nonempty-UI routes intentionally
      // choose different scene scissors and cannot serve as pixel baselines.
      std::array<ModelView,4> views;
      for(unsigned view=0;view<views.size();++view) {
        const auto rectangle=hud_viewport(4,view,true).outer;
        views[view].camera=Camera{};views[view].left=rectangle.left;views[view].top=rectangle.top;
        views[view].width=rectangle.right-rectangle.left;views[view].height=rectangle.bottom-rectangle.top;
        views[view].full_surface=true;
      }
      const auto capture=[&](const char* name,std::span<const Vertex> ui) {
        const auto path=files.path/(std::string(name)+".ppm");bool drawn{};
        for(unsigned retry=0;retry<4 && !drawn;++retry)drawn=renderer.draw(Camera{},path,{},false,{},{},ui,views);
        check(drawn,"Event-message framebuffer was not rendered");return read(path);
      };
      const auto baseline=capture("baseline",{});
      check(baseline.width==1280 && baseline.height==720,"Unexpected event-message drawable size");
      const auto verify_recipient=[&](const Image& image,unsigned recipient,const char* channel) {
        std::array<unsigned,4> counts{};
        bool valid=true;
        for(unsigned view=0;view<counts.size();++view) {
          counts[view]=changed(baseline,image,hud_viewport(4,view,true).outer);
          valid&=view==recipient?counts[view]>40:counts[view]==0;
        }
        if(!valid) {
          std::cerr<<"Event framebuffer channel="<<channel<<" recipient="<<recipient
              <<" vertices must affect only this cell; changed_pixels=["
              <<counts[0]<<','<<counts[1]<<','<<counts[2]<<','<<counts[3]<<"]\n";
          throw FormatError("Event pixels were blank or escaped the actual dead recipient's cell");
        }
      };
      for(unsigned recipient=0;recipient<4;++recipient) {
        std::uint8_t sequence{};SourceHudMessages bank;
        source_hud_message_submit(bank,sequence,state.tick,u"\ue044 \u00c9ric eliminated Player");
        const auto vertices=hud.draw(state,recipient,recipient,4,1280,720,25,14400,{},{},{},&bank);
        check(!vertices.empty(),"Dead event recipient produced no GPU glyph input");
        const auto image=capture("recipient",vertices);
        verify_recipient(image,recipient,"literal");
      }
      SourceHudPrimaryMessageTu15 primary{u"Sudden death!",state.tick,2,true};
      const auto primary_vertices=hud.draw_primary_message(primary,state,2,2,4,1280,720,constants,{});
      const auto primary_image=capture("primary",primary_vertices);
      verify_recipient(primary_image,2,"primary");
    }
    SDL_DestroyWindow(window);SDL_Quit();
  } catch(...) {SDL_DestroyWindow(window);SDL_Quit();throw;}
  std::cout<<"PASS GPU: nonblank dead-recipient text, four isolated HD cells, managed captures removed\n";
}
}

int main(int argc,char** argv)try {
  check(argc==2 || (argc==3 && std::string_view(argv[2])=="--gpu"),
      "Supply a prepared MCC map and optional --gpu");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  const auto original=OriginalData::native_tu15();
  check(bool(original->full_scoreboard_paint_constants),"Missing original primary text paint operands");
  Cache cache(argv[1],{},original);check(cache.is_mcc(),"Event rendering requires actual imported MCC assets");
  const auto fonts=gameplay_font_directory(cache);auto bank=std::make_shared<GameplayFontBank>(fonts);
  check(bank->source().domain==GameplayFontDomain::Mcc,"Event renderer selected a foreign font owner");
  const auto level=cache.load_content(0,true);
  auto content=cache.load_hud(level.player.datum,cache.slayer_starting_weapon());
  check(content.messages.enabled && content.messages.hold_seconds>0 && content.messages.fade_seconds>0,
      "Actual unit HUD lacks its message timing owner");
  content.action_messages.enabled=false;
  Mesh mesh;MultiplayerHud hud(content,fonts,mesh,4,"Slayer",{}, {},0,bank);
  MultiplayerState state;state.tick=90;
  for(unsigned n=0;n<state.players.size();++n) {
    auto& p=state.players[n];p.connected=n<4;p.health=p.shield=1;
    p.source_player=0x10000+n;p.source_unit=0x10000+n;p.source_membership=std::int8_t(n);
    p.position={float(n),0,0};
  }
  // Prime the actual radar material before death; an empty dead draw must
  // suppress both authored first-person widgets and the retained sensor page.
  hud.update_motion_sensor(state,0,0);
  const auto live=hud.draw(state,0,0,4,1280,720,25,14400);
  check(!live.empty(),"Death regression lacks live first-person artwork");
  for(unsigned n=0;n<4;++n)state.players[n].dead=true;
  check(hud.draw(state,0,0,4,1280,720,25,14400).empty(),
      "Dead player retained weapon, ammunition, score or radar artwork");

  unsigned glyph_draws{};
  // Source-authored medal codes and accented player-name characters are
  // rendered as their UTF16 glyphs, without UTF8 byte iteration or emoji.
  constexpr std::array<char16_t,12> characters{u'\ue033',u'\ue038',u'\ue044',u'\ue045',u'\ue046',
      u'\ue047',u'\ue048',u'\ue049',u'\ue051',u'\ue053',u'\u00e9',u'\u00f1'};
  for(unsigned players:{1u,2u,3u,4u})for(unsigned view=0;view<players;++view)
  for(const auto size:std::array<std::array<unsigned,2>,3>{{{640,480},{1280,720},{3840,2160}}}) {
    const auto index=players>1?1u:0u,selector=index?5u:6u;const auto font=bank->font(selector);
    const auto layout=hud_presentation_layout(hud_viewport(players,view,
        std::uint64_t(size[0])*3>std::uint64_t(size[1])*4),size[0],size[1]);
    for(char16_t c:characters) {
      const auto at=font->glyphs.find(c);check(at!=font->glyphs.end(),"Native event font lacks an authored glyph");
      const auto& glyph=at->second;check(glyph.width && glyph.height,"Authored event glyph has no visible geometry");
      SourceHudMessages messages;std::uint8_t sequence{};
      source_hud_message_submit(messages,sequence,state.tick,std::u16string_view(&c,1));
      const auto vertices=hud.draw(state,view,view,players,size[0],size[1],25,14400,{},{},{},&messages);
      check(vertices.size()==6,"Actual dead HUD did not draw exactly its one event glyph");
      const auto uv=retail_font_sample_uvs(*font,c,0,0,glyph.width,glyph.height);
      const float left=(layout.viewport.safe.left+glyph.bearing_x)*layout.scale+layout.offset.x;
      const float top=(layout.viewport.safe.top+(players>1?35.f:60.f)+font->ascent-glyph.bearing_y)*layout.scale+layout.offset.y;
      check(near(vertices[0].position.x*size[0]/640,left) && near(vertices[0].position.y*size[1]/480,top),
          "Event text lost its own corner viewport, original font metrics or glyph bearings");
      check(near(vertices[0].uv.x,uv[0]) && near(vertices[0].uv.y,uv[1]) &&
          near(vertices[1].uv.x,uv[2]) && near(vertices[2].uv.y,uv[3]),
          "Event medal/name glyph samples another atlas rectangle");
      const auto& material=mesh.materials.at(vertices[0].material);
      check(material.name=="action-message/font"+std::to_string(selector) &&
          material.diffuse.pixels==font->atlas.pixels,"Event glyph changed its actual native font6/5 pixel owner");
      const auto color=source_hud_message_color(false);
      for(const auto& v:vertices)check(v.position.z==1 && v.color.x==color[0] &&
          v.color.y==color[1] && v.color.z==color[2],"Dead event text changed its source HUD color or opacity");
      ++glyph_draws;
    }
  }
  // The optional local bank is a presentation input. It must neither publish
  // into the authoritative snapshot nor inherit another local player's row.
  check(state.players[0].source_messages==SourceHudMessages{},"Presentation override mutated a source snapshot");
  std::uint8_t sequence{};source_hud_message_submit(state.players[0].source_messages,sequence,state.tick,u"Victim");
  check(!hud.draw(state,0,0,4,1280,720,25,14400).empty() &&
      hud.draw(state,1,1,4,1280,720,25,14400).empty(),"Dead literal queue leaked across local players");
  SourceHudMessages empty;
  check(hud.draw(state,0,0,4,1280,720,25,14400,{},{},{},&empty).empty(),
      "Explicit empty presentation bank fell back to the source queue");
  state.tick+=90;
  const auto faded=hud.draw(state,0,0,4,1280,720,25,14400);
  check(!faded.empty() && faded.front().position.z>0 && faded.front().position.z<1,
      "Dead literal text failed to use the real simulation fade clock");
  state.tick+=90;
  check(hud.draw(state,0,0,4,1280,720,25,14400).empty(),"Expired death text still draws");
  state.tick=90;state.players[0].source_messages={};

  SourceHudPrimaryMessageTu15 primary{u"Sudden death!",90,0,true};
  const auto draw_primary=[&] {return hud.draw_primary_message(primary,state,0,0,4,1280,720,
      *original->full_scoreboard_paint_constants,{});};
  // Unchanged25B49B..25B4B3 compares clock<created+duration. Zero must not
  // acquire an invented default lifetime or enter the four-row literal bank.
  check(draw_primary().empty(),"Authored primary duration zero acquired a visible lifetime");
  primary.duration_seconds=2;
  const auto primary_start=draw_primary();check(!primary_start.empty(),"Positive source primary message failed to draw while dead");
  for(unsigned players:{1u,2u,3u,4u})for(unsigned view=0;view<players;++view) {
    const auto vertices=hud.draw_primary_message(primary,state,view,view,players,1280,720,
        *original->full_scoreboard_paint_constants,{});
    const auto outer=hud_viewport(players,view,true).outer;
    check(!vertices.empty(),"Primary event channel omitted a dead local recipient");
    for(const auto& v:vertices)check(mesh.materials.at(v.material).name==
        "action-message/font"+std::to_string(players>1?5:6) && v.position.x>=outer.left &&
        v.position.x<=outer.right && v.position.y>=outer.top && v.position.y<=outer.bottom,
        "Primary event channel lost its native font6/5 or recipient viewport");
  }
  state.tick=120;const auto primary_fade=draw_primary();
  const float expected=float(std::floor(255*std::pow(.5,double(.4f))))/255;
  check(!primary_fade.empty() && std::any_of(primary_fade.begin(),primary_fade.end(),[&](const auto& v) {
      return std::abs(v.position.z-expected)<.0001f;
    }),"Primary message lost the original0.4 fade exponent or source pixel alpha");
  state.tick=150;check(draw_primary().empty(),"Primary event text survived its strict source endpoint");
  state.tick=89;check(draw_primary().empty(),"Primary event text appeared before its creation tick");
  primary.created_tick=0x7ffffff0u;state.tick=primary.created_tick;
  check(draw_primary().empty(),"Primary signed endpoint wrap lost the original draw comparison");
  primary.created_tick=90;
  state.tick=90;primary.active=false;check(draw_primary().empty(),"Inactive primary event text still draws");
  state.players[0].connected=false;
  SourceHudMessages disconnected;source_hud_message_submit(disconnected,sequence,90,u"No recipient");
  check(hud.draw(state,0,0,4,1280,720,25,14400,{},{},{},&disconnected).empty(),
      "Disconnected event recipient retained message text");
  state.players[0].connected=true;
  std::cout<<"PASS CPU: "<<glyph_draws<<" native medal/locale draws,1-4 views at SD/HD/4K, death isolation, clocks and primary channels\n";
  if(argc==3)gpu(mesh,hud,state,*original->full_scoreboard_paint_constants);
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
