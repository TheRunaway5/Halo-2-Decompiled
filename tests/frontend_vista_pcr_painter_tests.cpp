#include "halo2/frontend.hpp"
#include "halo2/frontend_canvas.hpp"
#include "halo2/vista_pcr_appearance.hpp"
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
  content.vista=load_vista_settings(ui_map);const auto &pc=*content.vista;
  auto bank=original_bank(argv[3]);const auto before=*bank;
  const auto *sampled_environment=std::getenv("H2_TEST_MENU_ASSETS");
  const std::filesystem::path sampled=sampled_environment?sampled_environment:"";
  if(!sampled.empty() && !std::filesystem::is_regular_file(sampled/"menu-source.tsv"))return 77;
  Mesh mesh;FrontendPainter painter(content,maps/"fonts",mesh,{},
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
  if(owner->online) {
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
  nlohmann::json report={{"scope","Actual PC16/185 six panes, original retained fixture bank12, source assets/fonts and native Vulkan. Identity query fixture explicitly returns nonlocal. PC spas emblem rendering unresolved."},{"cases",nlohmann::json::array()}};
  unsigned cases{};
  for(const unsigned screen:{16,185})for(unsigned pane=0;pane<6;++pane) {
    MenuState state(content);state.open_vista_pcr(screen,owner);state.vista_pcr_pane(pane);
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
    state.players();check(details==++cases,"PCR Y details was not handled");
    std::optional<FrontendPointerTarget> done;
    for(float y=0;y<480 && !done;y+=2)for(float x=0;x<640 && !done;x+=2)
      if(const auto target=painter.pointer_hit(x,y);target && target->action==FrontendPointerAction::Back)done=target;
    check(done && state.activate_pointer(*done),"PCR Done/Close prompt cannot be clicked with a mouse");
  }
  check(closes==6 && continues==6,"PCR16 Done and PCR185 Close were conflated");
  check(*bank==before,"PCR painter changed retained source bank");
  if(argc>=5){report["viewports"]=viewports;report["online_vista_assets"]=owner->online;report["vulkan_validation"]=validation;std::ofstream output(std::filesystem::path(argv[4])/"report.json");output<<report.dump(2)<<'\n';}
  std::cout<<cases<<" actual-asset PCR panes, exact row owner counts, retained banks, source input actions and explicit PC shader gap passed\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
