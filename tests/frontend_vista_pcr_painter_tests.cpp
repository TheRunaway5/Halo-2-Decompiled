#include "halo2/frontend.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/original_pcr_intro.hpp"
#include "halo2/original_ui_map_intro.hpp"
#include "halo2/vista_pcr_appearance.hpp"
#include "halo2/vista_pcr_owner_state.hpp"
#include "halo2/desktop_session.hpp"
#include "halo2/renderer.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
std::u16string wide(const std::string &value){
  std::u16string result;for(std::size_t i=0;i<value.size();) {
    unsigned c=static_cast<unsigned char>(value[i++]),following{};
    if(c>=0xc0 && c<0xe0){c&=31;following=1;}
    else if(c>=0xe0 && c<0xf0){c&=15;following=2;}
    else if(c>=0xf0){c&=7;following=3;}
    while(following--){check(i<value.size(),"truncated UTF8 locale");c=(c<<6)|(static_cast<unsigned char>(value[i++])&63);}
    if(c<=0xffff)result+=char16_t(c);else {c-=0x10000;result+=char16_t(0xd800+(c>>10));result+=char16_t(0xdc00+(c&0x3ff));}
  }return result;
}
std::shared_ptr<VistaPcrResults> original_bank(const std::filesystem::path &fixture) {
  std::ifstream file(fixture);check(bool(file),"missing original PCR result fixture");
  std::string line;
  while(std::getline(file,line)) {
    if(line.empty() || line.front()=='#')continue;
    std::istringstream input(line);unsigned id{};std::string words,hex;input>>id>>words>>hex;
    if(id!=12)continue;
    std::replace(words.begin(),words.end(),',',' ');std::istringstream values(words);std::vector<std::uint32_t> q;
    std::uint32_t value;while(values>>value)q.push_back(value);
    check(q.size()>=12 && hex.size()==0x1940*2,"invalid retained original bank");
    auto bank=std::make_shared<VistaPcrResults>();bank->player_count=q[9];bank->team_count=q[10];bank->variant=q[11];
    unsigned cursor{};
    auto read=[&](auto &rows){for(auto &row:rows)for(auto &byte:row){byte=std::byte(std::stoul(hex.substr(cursor,2),nullptr,16));cursor+=2;}};
    read(bank->players);read(bank->teams);return bank;
  }
  throw std::runtime_error("original retained bank12 absent");
}
class Identity final:public VistaPcrAppearanceServices {
 public:
  unsigned queries{};
  bool identity_query(std::array<std::byte,8>)override{++queries;return false;}
  std::uint32_t primary_player_query(std::array<std::byte,8>)override{throw std::runtime_error("nonlocal fixture queried primary player");}
  bool player_query(std::uint32_t)override{throw std::runtime_error("nonlocal fixture queried player");}
};
std::u16string format(std::u16string_view source,std::span<const std::int32_t> values) {
  std::u16string result;unsigned argument{};
  for(unsigned i=0;i<source.size();++i) {
    if(source[i]!='%'){result+=source[i];continue;}
    ++i;check(i<source.size(),"truncated original format");
    if(source[i]=='%'){result+=u'%';continue;}
    const auto begin=i;while(i<source.size() && source[i]>='0' && source[i]<='9')++i;
    check(i<source.size() && source[i]=='d' && argument<values.size(),"unsupported original PCR format");
    auto number=wide(std::to_string(values[argument++]));
    if(begin<i && source[begin]=='0') {
      unsigned width{};for(unsigned j=begin;j<i;++j)width=width*10+source[j]-u'0';
      if(number.size()<width)number.insert(number.begin(),width-number.size(),u'0');
    }
    result+=number;
  }
  check(argument==values.size(),"original PCR format arguments unused");return result;
}
}
int main(int argc,char **argv)try {
  if(argc<4 || argc>6 || (argc==6 && std::string(argv[5])!="--widescreen") || !std::filesystem::is_regular_file(std::filesystem::path(argv[1])/"mainmenu.map"))return 77;
  std::filesystem::path ui_map=argv[2];
  if(std::filesystem::is_regular_file(ui_map/"menu-source.tsv"))
    ui_map=hud_menu_asset_directory(ui_map)/"maps/mainmenu.map";
  if(!std::filesystem::is_regular_file(ui_map))return 77;
  const std::filesystem::path maps=argv[1];Cache cache(maps/"mainmenu.map");auto content=cache.load_frontend();
  const bool require_mcc_intro=std::getenv("H2_TEST_PCR_INTRO_MCC");
  if(require_mcc_intro && !std::getenv("DISPLAY"))return 77;
  check(!require_mcc_intro || cache.is_mcc(),"PCR intro GPU qualification requires admitted MCC UI assets");
  content.vista=load_vista_settings(ui_map);const auto &pc=*content.vista;
  auto bank=original_bank(argv[3]);const auto before=*bank;
  const auto *sampled_environment=std::getenv("H2_TEST_MENU_ASSETS");
  const std::filesystem::path sampled=sampled_environment?sampled_environment:"";
  if(!sampled.empty() && !std::filesystem::is_regular_file(sampled/"menu-source.tsv"))return 77;
  Mesh mesh;FrontendPainter painter(content,gameplay_font_directory(cache),mesh,{},
      load_frontend_ui_assets(content,sampled),sampled.empty()?sampled:sampled/"fonts");
  std::vector<VistaPcrRgb> palette;for(const auto &color:pc.appearance_palette)
    palette.push_back({std::bit_cast<std::uint32_t>(color.x),std::bit_cast<std::uint32_t>(color.y),std::bit_cast<std::uint32_t>(color.z)});
  VistaPcrAppearanceAssets assets;assets.globals_present=true;assets.appearance_palette=palette;
  std::vector<VistaPcrRgb> teams;for(const auto &color:pc.vista_pcr_team_palette)
    teams.push_back({std::bit_cast<std::uint32_t>(color.x),std::bit_cast<std::uint32_t>(color.y),std::bit_cast<std::uint32_t>(color.z)});
  assets.team_palette=teams;assets.rank_owner_present=pc.vista_pcr_rank_owner_present;
  assets.rank_ranges=pc.vista_pcr_rank_ranges;
  check(teams.size()==9 && assets.rank_owner_present && assets.rank_ranges.size()==50,"actual PCR appearance asset owners were not imported");
  const VistaLoadingFloatingPoint floating_point{0x27f,0x1f80};
  Identity identity;
  auto owner=std::make_shared<VistaPcrScreenOwner>();owner->results=bank;owner->selected_row=0;
  const auto *online_environment=std::getenv("H2_TEST_ONLINE_UI");
  owner->online=online_environment && std::string(online_environment)=="1";
  std::vector<Texture> vista_fonts;
  std::vector<bool> vista_bitmaps(mesh.materials.size());
  if(owner->online && !require_mcc_intro) {
    std::ifstream table(pc.source_fonts/"font_table.txt");
    check(bool(table),"Missing actual Vista PCR font table");
    std::string name;
    while(std::getline(table,name)) {
      if(!name.empty() && name.back()=='\r')name.pop_back();
      if(name.empty())continue;
      for(auto &ch:name)if(ch>='A' && ch<='Z')ch+='a'-'A';
      vista_fonts.push_back(load_retail_font(pc.source_fonts/name).atlas);
    }
    check(!vista_fonts.empty(),"Missing actual Vista PCR font bank");
    const auto bank=load_frontend_ui_assets(content,pc.source_fonts.parent_path().parent_path());
    for(std::size_t i=0;i<mesh.materials.size();++i) {
      const auto &material=mesh.materials[i];
      const auto same=[&](const Texture &texture) {
        return texture.width==material.diffuse.width && texture.height==material.diffuse.height &&
          texture.format==material.diffuse.format && texture.pixels==material.diffuse.pixels;
      };
      if(const auto entry=bank.find(material.name);entry!=bank.end())
        vista_bitmaps[i]=std::any_of(entry->second.begin(),entry->second.end(),same);
      for(const auto &[id,image]:pc.images)
        if(image.name==material.name && std::any_of(image.images.begin(),image.images.end(),same))
          vista_bitmaps[i]=true;
    }
  }
  owner->floating_point=floating_point;
  owner->rows.selected_row=[] {return 0u;};
  owner->rows.localized=[&](std::uint32_t key){return wide(pc.localized(pc.screens.at(16),key));};
  owner->rows.format=format;
  owner->rows.team_color=[&](std::uint16_t team){const auto color=vista_pcr_appearance_colors({},team,0,assets)[0];
    return VistaPcrTeamColor{vista_pcr_profile_name_color(color,floating_point),color};};
  owner->appearance=[&](unsigned row){
    const auto &pane=pc.screens.at(16).panes.at(1);const auto &outer=pc.skins.at(pane.lists.front().skin);
    const auto &skin=pc.skin_groups.at(outer.grids.front().skin);
    assets.emblem_definition_flags=skin.emblems.front().flags;
    for(const auto &bitmap:skin.bitmaps) {
      const auto present=pc.images.contains(bitmap.datum) && !pc.images.at(bitmap.datum).images.empty();
      if(bitmap.placeholder==0x14000698){assets.primary_bitmap_present=present;assets.primary_page_count=present?pc.images.at(bitmap.datum).images.size():0;assets.previous_primary_frame=bitmap.frame;}
      if(bitmap.placeholder==0x0900069a){assets.rank_bitmap_present=present;assets.rank_page_count=present?pc.images.at(bitmap.datum).images.size():0;assets.previous_rank_frame=bitmap.frame;}
    }
    const auto view=vista_pcr_player_presentation(*bank,row,assets,identity,floating_point);
    return VistaPcrPlayerPresentation{view.appearance,view.team,view.rank_range,view.primary_color,view.name_color,
        view.primary_visible,view.secondary_visible,view.rank_visible,view.emblem_visible,view.primary_frame,view.rank_frame};
  };
  owner->select_row=[](unsigned){};
  // Explicit original616CFC fixture service: no voice slot for these XUIDs.
  // This controls only this rendering test and never publishes session data.
  owner->voice=[&](unsigned row){VistaPcrVoiceServices services;
    services.identity_slot=[](const auto &){return 0xffffffffu;};
    return vista_pcr_voice(*bank,row,0,services);};
  owner->elapsed=[](std::uint32_t){return 4000u;};
  unsigned closes{},continues{},details{};
  owner->input.host=[] {return std::uint8_t(1);};owner->input.details_available=[] {return std::uint8_t(1);};
  owner->input.selected_identity=[](auto &value){value={0x12345678,0x9abcdef0};return std::uint8_t(1);};
  owner->input.player_details=[&](const auto &value){check(value[0]==0x12345678,"selected identity lost");++details;};
  owner->input.close=[&](unsigned channel){check(channel==3,"incorrect PCR close channel");++closes;};
  owner->input.continue_session=[&]{++continues;};owner->input.dialog=[](const auto &){throw std::runtime_error("host fixture entered client dialog");};
  std::unique_ptr<DesktopSession> desktop;std::unique_ptr<Renderer> renderer;
  const auto *validation_environment=std::getenv("H2_TEST_VK_VALIDATION");
  const bool validation=!validation_environment || std::string(validation_environment)!="0";
  if(argc>=5){std::filesystem::create_directories(argv[4]);desktop=std::make_unique<DesktopSession>();renderer=std::make_unique<Renderer>(desktop->window(1280,720,true),mesh,validation);}
  const std::vector<std::pair<unsigned,unsigned>> viewports=argc==6?
      std::vector<std::pair<unsigned,unsigned>>{{640,480},{1280,720},{1280,800},{1920,1080},{2560,1080},{3440,1440},{3840,1080},{2560,1437},{1001,601}}:
      std::vector<std::pair<unsigned,unsigned>>{{1280,720}};
  nlohmann::json report={{"scope",require_mcc_intro?
    "Actual native PCR16 full-window intro over admitted MCC UI, original retained reference numeric bank12; controlled original clock service; native Vulkan pixels; prior185 bypass.":
    "Actual PC16/185 six panes, original retained fixture bank12, source assets/fonts and native Vulkan. Identity query fixture explicitly returns nonlocal. PC spas emblem rendering unresolved."},{"cases",nlohmann::json::array()}};
  unsigned cases{};
  if(!require_mcc_intro)for(const unsigned screen:{16,185})for(unsigned pane=0;pane<6;++pane) {
    MenuState state(content);std::uint32_t pcr_clock=0x12345678;
    state.set_pcr_clock([&]{return pcr_clock;});state.open_vista_pcr(screen,owner);
    const auto opened=state.frame().pcr_open_clock;pcr_clock+=4000;
    state.vista_pcr_pane(pane);
    check(state.frame().pcr_open_clock==opened,"PCR pane selection restarted its original screen clock");
    const auto old_queries=identity.queries;const auto draw=painter.draw(state,5,-1,10000);
    check(!draw.vertices.empty(),"PCR pane produced no draw");
    check(identity.queries-old_queries==(pane?bank->player_count:0),"PCR appearance owner is not consumed exactly once per player row");
    check(draw.rendering_gaps.size()==(pane?1u:0u),"PCR unresolved PC emblem gap was lost or persisted into team pane");
    unsigned name_vertices{};
    for(const auto &v:draw.vertices) {
      check(std::isfinite(v.position.x) && std::isfinite(v.position.y) && v.material<mesh.materials.size(),"invalid PCR vertex");
      check(!mesh.materials[v.material].ui_emblem_pass,"Xbox emblem pass substituted for actual PC shader");
      if(pane && mesh.materials[v.material].name.starts_with("font/") &&
          v.color.x>.29f && v.color.x<.31f && v.color.y==v.color.x && v.color.z==v.color.x)++name_vertices;
      if(mesh.materials[v.material].name.starts_with("font/"))
        check(v.position.x>=0 && v.position.x<=640 && v.position.y>=0 && v.position.y<=480 &&
              !mesh.materials[v.material].ui_widescreen_background,"PCR text escapes or stretches with the widescreen background");
      if(owner->online && mesh.materials[v.material].name.starts_with("font/")) {
        const auto &texture=mesh.materials[v.material].diffuse;
        check(std::any_of(vista_fonts.begin(),vista_fonts.end(),[&](const auto &font) {
          return font.width==texture.width && font.height==texture.height && font.format==texture.format && font.pixels==texture.pixels;
        }),"ONLINE PCR inherited the selected shell's font atlas");
      }
      if(owner->online && mesh.materials[v.material].name.starts_with("ui\\") &&
          mesh.materials[v.material].name.find("/native-")==std::string::npos)
        check(vista_bitmaps[v.material],"ONLINE PCR inherited the selected shell's bitmap texture");
      if(!owner->online && !sampled.empty() && mesh.materials[v.material].name.starts_with("font/"))
        check(mesh.materials[v.material].diffuse.width>1024,"PCR text failed to select imported font raster");
    }
    if(pane)check(name_vertices>0,"PCR assigned player names produced no glyph vertices");
    const auto stem="pcr"+std::to_string(screen)+"-pane"+std::to_string(pane);
    if(renderer)for(const auto [width,height]:viewports) {
      check(desktop->apply_video(width,height,false).empty(),"PCR capture mode change failed");
      SDL_PumpEvents();renderer->resized();
      const auto canvas=frontend_canvas(width,height);
      for(float y=4;y<480;y+=8)for(float x=4;x<640;x+=8) {
        const auto target=painter.pointer_hit(x,y);
        if(!target || painter.pointer_hit(x-1,y)!=target || painter.pointer_hit(x+1,y)!=target ||
            painter.pointer_hit(x,y-1)!=target || painter.pointer_hit(x,y+1)!=target)continue;
        const float px=std::floor(canvas.left+x*canvas.scale)+.5f,py=std::floor(canvas.top+y*canvas.scale)+.5f;
        check(painter.pointer_hit((px-canvas.left)/canvas.scale,(py-canvas.top)/canvas.scale)==target,
              "PCR mouse target drifts in widescreen");
      }
      const auto name=stem+(argc==6?"-"+std::to_string(width)+"x"+std::to_string(height):"")+".ppm";
      bool captured{};
      for(unsigned attempt=0;attempt<3 && !captured;++attempt)
        captured=renderer->draw(Camera{},std::filesystem::path(argv[4])/name,{},false,{},{},draw.vertices,{}, {},draw.backdrops);
      check(captured,"native PCR capture failed");
    }
    report["cases"].push_back({{"name",stem},{"vertices",draw.vertices.size()},{"name_vertices",name_vertices},{"rows",state.frame().entries.size()},{"rendering_gaps",draw.rendering_gaps}});
    check(state.frame().context==pane && state.stack().size()==1,"PCR pane/factory ownership drifted");
    state.navigate(1,0);
    check(state.frame().context==vista_pcr_tab_step(pane,1,bank->team_count>0),"PCR keyboard Right did not reach the next visible tab");
    state.navigate(-1,0);
    check(state.frame().context==pane,"PCR keyboard Left did not return to its tab");
    const auto find_prompt=[&](FrontendPointerAction action,unsigned index=0)->std::optional<FrontendPointerTarget> {
      for(float y=0;y<480;y+=2)for(float x=0;x<640;x+=2)
        if(const auto target=painter.pointer_hit(x,y);target && target->action==action && target->index==index)return target;
      return {};
    };
    painter.draw(state,5,-1,10000);
    const auto next=find_prompt(FrontendPointerAction::PcrTab,1);
    check(next && state.activate_pointer(*next),"PCR authored right arrow cannot be clicked");
    check(state.frame().context==vista_pcr_tab_step(pane,1,bank->team_count>0),"PCR mouse Right did not reach the next visible tab");
    check(!state.activate_pointer(*next),"PCR stale arrow target changed another tab");
    painter.draw(state,5,-1,10000);
    const auto previous=find_prompt(FrontendPointerAction::PcrTab,0);
    check(previous && state.activate_pointer(*previous) && state.frame().context==pane,"PCR authored left arrow cannot return to its tab");
    painter.draw(state,5,-1,10000);
    state.players();check(details==++cases,"PCR Y details was not handled");
    std::optional<FrontendPointerTarget> done;
    for(float y=0;y<480 && !done;y+=2)for(float x=0;x<640 && !done;x+=2)
      if(const auto target=painter.pointer_hit(x,y);target && target->action==FrontendPointerAction::Back)done=target;
    check(done && state.activate_pointer(*done),"PCR Done/Close prompt cannot be clicked with a mouse");
  }
  check(closes==(require_mcc_intro?0u:6u) && continues==(require_mcc_intro?0u:6u),"PCR16 Done and PCR185 Close were conflated");
  // Keep the report allocation deliberately old. Only the actual initialized
  // screen's monotonic service starts233423/23A0F3's intro lifetime.
  MenuState intro(content);std::uint32_t pcr_clock=0xfffffff0u;
  intro.set_pcr_clock([&]{return pcr_clock;});intro.open_vista_pcr(16,owner);
  check(intro.frame().pcr_open_clock==pcr_clock,"PCR actual open did not sample its native clock service");
  const auto opened=intro.frame().pcr_open_clock;
  bool rejected_clock{};try{intro.set_pcr_clock([]{return 0u;});}catch(const FormatError&){rejected_clock=true;}
  check(rejected_clock,"Open PCR accepted a different elapsed-time owner");
  const auto capture_intro=[&](const UiDraw &draw,const std::string &name) {
    std::vector<unsigned char> pixels;if(!renderer)return pixels;
    check(desktop->apply_video(1280,720,false).empty(),"PCR intro capture mode change failed");
    SDL_PumpEvents();renderer->resized();
    const auto path=std::filesystem::path(argv[4])/(name+".ppm");bool captured{};
    for(unsigned i=0;i<3&&!captured;++i)captured=renderer->draw(Camera{},path,{},false,{},{},draw.vertices,{},{},draw.backdrops);
    check(captured,"Original PCR intro did not reach native Vulkan output");
    std::ifstream input(path,std::ios::binary);std::string kind;unsigned width,height,maximum;
    input>>kind>>width>>height>>maximum;input.get();
    check(kind=="P6" && width==1280 && height==720 && maximum==255,"PCR intro GPU framebuffer is invalid");
    pixels.resize(width*height*3);input.read(reinterpret_cast<char*>(pixels.data()),pixels.size());
    check(bool(input),"PCR intro GPU framebuffer is truncated");return pixels;
  };
  nlohmann::json intro_report={{"source","Xbox TU1.5 23A0F3 /233423"},
      {"asset_source",cache.is_mcc()?"admitted MCC":"offline reference presentation"},
      {"actual_open_clock",opened},{"report_allocation_clock_ignored",true},{"cases",nlohmann::json::array()}};
  std::vector<unsigned char> black,middle,clear,prior;
  for(const auto elapsed:{0u,1999u,2000u,3000u,3999u,4000u}) {
    pcr_clock=opened+elapsed;
    const auto color=intro.pcr_intro_color(intro.frame());
    const auto expected=original_pcr_intro_color(false,opened,pcr_clock,floating_point.x87_control_word,floating_point.mxcsr);
    check(color==expected,"Menu PCR used allocation/UI clock instead of actual source clock");
    const auto draw=painter.draw(intro,5,-1,10000);
    unsigned vertices{};
    for(const auto &v:draw.vertices)if(mesh.materials.at(v.material).name=="ui/original-pcr-intro-black") {
      ++vertices;check(mesh.materials.at(v.material).ui_full_surface &&
          std::abs(v.position.z-float(*color>>24)/255.f)<.000001f,
          "PCR intro lost its actual full-window ARGB alpha");
    }
    check(vertices==((*color>>24)?6u:0u),"PCR intro draw count differs from source packed alpha");
    const auto name="pcr-original-intro-"+std::to_string(elapsed);
    const auto pixels=capture_intro(draw,name);
    if(elapsed==0)black=pixels;else if(elapsed==3000)middle=pixels;else if(elapsed==4000)clear=pixels;
    if(renderer && elapsed<=2000)check(pixels==black,"Original PCR opaque hold leaked rendered children before2000ms");
    intro_report["cases"].push_back({{"elapsed",elapsed},{"argb",*color},{"overlay_vertices",vertices},{"frame",name+".ppm"}});
  }
  intro.navigate(1,0);check(intro.frame().pcr_open_clock==opened,"Original tab input restarted the fade");
  intro.open_vista_pcr(185,owner);pcr_clock+=100000;
  check(!intro.pcr_intro_color(intro.frame()),"Original prior statistics did not bypass the intro at open");
  const auto prior_draw=painter.draw(intro,5,-1,10000);
  check(std::none_of(prior_draw.vertices.begin(),prior_draw.vertices.end(),[&](const auto &v){
    return mesh.materials.at(v.material).name=="ui/original-pcr-intro-black";}),"PCR185 submitted an intro-black quad");
  prior=capture_intro(prior_draw,"pcr-original-prior-bypass");
  if(renderer) {
    std::uint64_t hold_energy{},middle_energy{},clear_energy{};unsigned visible{},darker{},prior_visible{};
    // Upper-channel Friends footer follows the source PCR window. Compare
    // the actual full-width body above that independent channel's region.
    for(unsigned y=0;y<600;++y)for(unsigned x=0;x<1280;++x)for(unsigned c=0;c<3;++c) {
      const auto at=(y*1280+x)*3+c;hold_energy+=black[at];middle_energy+=middle[at];clear_energy+=clear[at];
      if(clear[at]>24){++visible;darker+=middle[at]<clear[at];}
      prior_visible+=prior[at]>24;
    }
    check(hold_energy==0,"Original opaque PCR intro left a letterbox or row visible");
    check(middle_energy>0 && middle_energy<clear_energy && visible>1000 && darker>visible*9/10,
          "Original PCR middle alpha did not visibly fade its actual MCC children");
    check(prior_visible>1000,"Prior statistics incorrectly inherited an opaque intro");
    intro_report["gpu"]={{"passed",true},{"hold_energy",hold_energy},{"middle_energy",middle_energy},
      {"clear_energy",clear_energy},{"visible_channels",visible},{"darker_channels",darker},{"prior_visible_channels",prior_visible}};
  }
  intro_report["prior_bypass"]=true;intro_report["pane_does_not_restart_clock"]=true;
  report["original_intro"]=std::move(intro_report);
  // The real new-map callback owns a second, independent accumulated UI
  // clock. It renders after all channels, including the Friends footer.
  MenuState overlap(content);std::uint32_t overlap_native=0x12345678u;
  overlap.set_pcr_clock([&]{return overlap_native;});
  const std::uint32_t map_initialized=0xfffffff0u;
  overlap.ui_clock(map_initialized);overlap.initialize_mainmenu_map();
  overlap.open_vista_pcr(16,owner);
  const auto overlap_opened=overlap.frame().pcr_open_clock;
  nlohmann::json map_report={{"source","Xbox TU1.5 14A38B/14A500/14A70D plus independent PCR23A0F3"},
      {"map_ui_clock",map_initialized},{"pcr_native_open_clock",overlap_opened},
      {"cases",nlohmann::json::array()}};
  const auto capture_overlap=[&](std::uint32_t ui_elapsed,std::uint32_t native_elapsed,const std::string &name) {
    overlap.ui_clock(map_initialized+ui_elapsed);overlap_native=overlap_opened+native_elapsed;
    const auto map_color=overlap.ui_map_intro_color();const auto pcr_color=overlap.pcr_intro_color(overlap.frame());
    const auto draw=painter.draw(overlap,5,-1,10000);
    unsigned map_vertices{};
    for(const auto &v:draw.vertices)if(mesh.materials.at(v.material).name=="ui/original-map-intro-black") {
      ++map_vertices;check(map_color && mesh.materials.at(v.material).ui_full_surface &&
          std::abs(v.position.z-float(*map_color>>24)/255.f)<.000001f,
          "New-map intro lost its actual source full-window alpha");
    }
    check(map_vertices==(map_color?6u:0u),"New-map intro draw count differs from14A62E");
    if(map_vertices)check(mesh.materials.at(draw.vertices.back().material).name=="ui/original-map-intro-black",
        "New-map intro did not render after all ordinary UI channels");
    map_report["cases"].push_back({{"ui_elapsed",ui_elapsed},{"native_elapsed",native_elapsed},
        {"map_argb",map_color?nlohmann::json(*map_color):nlohmann::json(nullptr)},
        {"pcr_argb",pcr_color?nlohmann::json(*pcr_color):nlohmann::json(nullptr)},
        {"frame",name+".ppm"}});
    return capture_intro(draw,name);
  };
  const auto map_black=capture_overlap(0,4000,"map-original-intro-black");
  const auto map_hold=capture_overlap(3000,3000,"map-original-intro-hold");
  const auto map_both=capture_overlap(3500,3000,"map-original-intro-overlap");
  const auto map_only=capture_overlap(3500,4000,"map-original-intro-only");
  const auto pcr_only=capture_overlap(4001,3000,"map-original-pcr-only");
  const auto map_clear=capture_overlap(4001,4000,"map-original-intro-clear");
  check(!overlap.ui_map_intro_color() && (*overlap.pcr_intro_color(overlap.frame())>>24)==0,
      "Combined intro clear did not require both source clocks");
  overlap.ui_clock(map_initialized+3500);const auto before_tab=overlap.ui_map_intro_color();
  overlap.navigate(1,0);check(overlap.ui_map_intro_color()==before_tab,
      "PCR tab navigation restarted the global map intro");
  overlap.open_vista_pcr(185,owner);
  check(!overlap.pcr_intro_color(overlap.frame()) && overlap.ui_map_intro_color()==before_tab,
      "Prior statistics bypass changed the independent real map lifecycle");
  overlap.ui_clock(map_initialized+4001);
  const auto map_prior=capture_overlap(4001,4000,"map-original-prior-clear");
  if(renderer) {
    check(std::all_of(map_black.begin(),map_black.end(),[](auto value){return value==0;}) && map_black==map_hold,
        "Global map intro failed to cover full framebuffer/Friends footer through3000ms");
    std::uint64_t both_energy{},map_energy{},pcr_energy{},clear_energy{};
    for(unsigned y=0;y<600;++y)for(unsigned x=0;x<1280;++x)for(unsigned channel=0;channel<3;++channel) {
      const auto i=(y*1280+x)*3+channel;both_energy+=map_both[i];map_energy+=map_only[i];
      pcr_energy+=pcr_only[i];clear_energy+=map_clear[i];
    }
    check(both_energy>0 && both_energy<map_energy && both_energy<pcr_energy &&
        map_energy<clear_energy && pcr_energy<clear_energy,
        "Independent original map/PCR intro overlap did not reach actual MCC pixels");
    check(std::count_if(map_prior.begin(),map_prior.end(),[](auto value){return value>24;})>1000,
        "Prior185 stayed black after the actual map clock cleared");
    map_report["gpu"]={{"passed",true},{"both_energy",both_energy},{"map_only_energy",map_energy},
        {"pcr_only_energy",pcr_energy},{"clear_energy",clear_energy},{"full_surface_opaque",true}};
  }
  map_report["independent_clocks"]=true;map_report["tab_and_prior_do_not_reset_map"]=true;
  report["original_map_intro"]=std::move(map_report);
  check(*bank==before,"PCR painter changed retained source bank");
  if(argc>=5){report["viewports"]=viewports;report["online_vista_assets"]=owner->online;report["vulkan_validation"]=validation;std::ofstream output(std::filesystem::path(argv[4])/"report.json");output<<report.dump(2)<<'\n';}
  std::cout<<cases<<" actual-asset PCR panes, exact row owner counts, retained banks, source input actions and explicit PC shader gap passed\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
