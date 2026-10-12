#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/original_data.hpp"
#include "halo2/ui_dds.hpp"
#include "halo2/frontend_emblem.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <bit>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Texture dds(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);const auto size=input.tellg();
  check(bool(input)&&size>=128&&size<=1024*1024,"Bounded Vista scoreboard page missing");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));input.seekg(0);
  input.read(reinterpret_cast<char *>(bytes.data()),size);
  return read_ui_dds_texture(Bytes(bytes));
}
bool same(const Texture &a,const Texture &b) {
  return a.width==b.width&&a.height==b.height&&a.format==b.format&&a.pixels==b.pixels;
}
}
int main(int argc,char **argv)try {
  if(argc!=4)throw std::runtime_error("Supply original map, original executable and Vista artwork root");
  if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2])||!std::filesystem::exists(argv[3]))return 77;
  const std::filesystem::path map=argv[1],root=argv[3];
  const Cache cache(map,{},OriginalData::load_xbe(argv[2]));
  Mesh stock_mesh,mesh;UiVariant variant;
  MultiplayerScoreboard stock(cache,map.parent_path()/"fonts",stock_mesh,variant);
  MultiplayerScoreboard vista(cache,map.parent_path()/"fonts",mesh,variant,root);
  const auto source_assets=cache.load_full_scoreboard_assets();
  const auto source_emblem=frontend_emblem_material(cache,source_assets.emblem_shader);
  const auto source_death=cache.load_bitmap(source_assets.dead_bitmap,0);
  unsigned original_emblems=0,original_deaths=0;
  for(const auto &m:stock_mesh.materials) {
    if(m.name=="scoreboard/original/emblem") {
      check(m.ui_full_surface&&m.ui_emblem_pass&&m.hud_pass,"Original emblem bypassed native HUD renderer");
      check(m.forward_program.words==source_emblem.forward_program.words,"Original scoreboard changed the authored emblem program");
      for(unsigned stage=0;stage<2;++stage) {
        check(m.animated_textures[stage].size()==source_emblem.animated_textures[stage].size(),"Original scoreboard lost emblem pages");
        for(unsigned page=0;page<m.animated_textures[stage].size();++page)
          check(same(m.animated_textures[stage][page],source_emblem.animated_textures[stage][page]),"Original scoreboard substituted an emblem page");
      }
      ++original_emblems;
    } else if(m.name=="scoreboard/original/death") {
      check(m.ui_full_surface&&m.hud_pass&&same(m.diffuse,source_death),"Original scoreboard substituted its death icon");
      ++original_deaths;
    }
  }
  check(original_emblems==4*multiplayer_capacity&&original_deaths==original_emblems,"Original scoreboard needs independent player/view materials");
  std::map<std::string,std::vector<Texture>> pages;
  std::ifstream manifest(root/"scoreboard.tsv");std::string line;std::getline(manifest,line);
  while(std::getline(manifest,line)) {
    std::istringstream row(line);std::array<std::string,6> field;
    for(auto &value:field)check(bool(std::getline(row,value,'\t')),"Incomplete Vista scoreboard manifest");
    auto &bank=pages[field[0]];bank.resize(std::stoul(field[2]));bank.at(std::stoul(field[1]))=dds(root/field[5]);
  }
  const auto &background=pages.at("ui\\global_bitmaps\\emblems\\background"),
      &foreground=pages.at("ui\\global_bitmaps\\emblems\\foreground"),
      &death=pages.at("ui\\hud\\bitmaps\\new_hud\\dead_friendly_arrow");
  check(background.size()==32&&foreground.size()==64&&death.size()==1,"Complete Vista scoreboard banks missing");
  unsigned bindings=0;
  for(const auto &m:mesh.materials) {
    if(m.name=="scoreboard/vista/emblem") {
      check(m.ui_full_surface&&m.ui_emblem_pass&&m.hud_pass,"Emblem bypassed native Vulkan HUD renderer");
      check(m.animated_textures[0].size()==32&&m.animated_textures[1].size()==64,"Vista scoreboard animation bank lost");
      for(unsigned i=0;i<32;++i)check(same(m.animated_textures[0][i],background[i]),"Scoreboard background page is not Vista DDS");
      for(unsigned i=0;i<64;++i)check(same(m.animated_textures[1][i],foreground[i]),"Scoreboard foreground page is not Vista DDS");
      bindings+=96;
    } else if(m.name=="scoreboard/vista/death") {
      check(m.ui_full_surface&&m.hud_pass&&same(m.diffuse,death[0]),"Scoreboard death icon is not Vista DDS");++bindings;
    }
  }
  check(bindings==4*multiplayer_capacity*97,"Every local scoreboard requires independent Vista materials");
  MultiplayerState state;
  for(unsigned i=0;i<4;++i) {
    auto &p=state.players[i];p.connected=true;p.name="Player"+std::to_string(i);
    p.source_player=0x10000+i;p.source_unit=i==1?null_datum:0x20000+i;
    p.source_flags=1;p.source_membership=std::int8_t(i);
    const std::array<std::uint8_t,9> appearance{10,0,2,3,0,std::uint8_t(7+i),std::uint8_t(11+i),std::uint8_t(i&1),0};
    p.source_configuration=native_selected_player_configuration(p.name,appearance,-1);
  }
  const std::array<unsigned,4> ids{0,1,2,3};unsigned cases=0;
  for(unsigned count=1;count<=4;++count)for(unsigned i=0;i<count;++i)
    for(unsigned tick=0;tick<8;++tick){stock.step(i,true);vista.step(i,true);}
  for(unsigned count=1;count<=4;++count)for(bool wide:{false,true})for(unsigned view=0;view<count;++view) {
    state.finished=false;state.results_finalized=false;
    for(bool final:{false,true}) {
      state.finished=state.results_finalized=final;
      const auto a=stock.draw(state,std::span(ids).first(count),view,wide?1280:640,wide?720:480),
          b=vista.draw(state,std::span(ids).first(count),view,wide?1280:640,wide?720:480);
      check(!a.normal_hud&&!a.paint.emblems_unavailable&&a.paint.status==FullScoreboardPaintStatus::Ready,"Original scoreboard omitted its authored emblem/death materials");
      check(!b.normal_hud&&!b.paint.emblems_unavailable&&b.paint.status==FullScoreboardPaintStatus::Ready,"Full Vista scoreboard failed admission");
      check(a.layout.rows.size()==b.layout.rows.size(),"Vista changed scoreboard roster");
      for(unsigned row=0;row<a.layout.rows.size();++row)for(unsigned widget=0;widget<6;++widget) {
        const auto &x=a.layout.rows[row].widgets[widget],&y=b.layout.rows[row].widgets[widget];
        check(x.valid==y.valid&&x.text==y.text,"Vista changed scoreboard content");
        check(x.rectangle.top==y.rectangle.top&&x.rectangle.left==y.rectangle.left&&x.rectangle.bottom==y.rectangle.bottom&&x.rectangle.right==y.rectangle.right,"Vista moved current scoreboard layout");
      }
      unsigned icons=0;
      for(const auto &v:b.paint.draw.vertices) {
        const auto &m=mesh.materials.at(v.material);
        if(!m.name.starts_with("scoreboard/vista/"))continue;
        ++icons;
        if(m.name.ends_with("emblem"))check(m.texture_frames[0]>=11&&m.texture_frames[0]<=14&&m.texture_frames[1]>=7&&m.texture_frames[1]<=10,"Wrong player emblem pages on scoreboard");
      }
      check(icons==24,"Both scoreboard routes must submit every admitted player icon");++cases;
      unsigned stock_icons=0,stock_deaths=0;
      for(const auto &v:a.paint.draw.vertices) {
        const auto &m=stock_mesh.materials.at(v.material);
        if(!m.name.starts_with("scoreboard/original/"))continue;
        ++stock_icons;
        if(m.name.ends_with("death")) {
          // Original2325CA binds RGB(1,0,0); source extern33 maps to C0[2].
          check((m.forward_program.words[12]&0xffffffu)==0xff0000u,"Original scoreboard death icon lost its red waypoint-color extern");
          ++stock_deaths;continue;
        }
        check(m.texture_frames[0]>=11&&m.texture_frames[0]<=14&&m.texture_frames[1]>=7&&m.texture_frames[1]<=10,"Original scoreboard selected another player's emblem");
        check(m.texture_frames[0]-11==m.texture_frames[1]-7,"Original scoreboard mixed foreground/background owners");
        const auto slot=m.texture_frames[1]-7;
        FrontendAppearance appearance;
        const auto &configuration=*state.players[slot].source_configuration;
        for(unsigned color=0;color<4;++color)appearance.colors[color]=std::bit_cast<std::int8_t>(configuration[0x40+color]);
        appearance.foreground=configuration[0x45];appearance.background=configuration[0x46];appearance.flags=configuration[0x47];
        std::vector<Vec3> palette;
        for(const auto &color:source_assets.palette)palette.push_back(std::bit_cast<Vec3>(color));
        auto expected=source_emblem;update_frontend_emblem(expected,appearance,palette,1);
        check(m.object_render_colors&&expected.object_render_colors&&
            std::bit_cast<std::array<std::uint32_t,12>>(*m.object_render_colors)==
            std::bit_cast<std::array<std::uint32_t,12>>(*expected.object_render_colors),
            "Original scoreboard used another player's emblem colors");
      }
      const auto dead_rows=std::count_if(a.layout.rows.begin(),a.layout.rows.end(),[](const auto &row){return row.icon_kind==1;});
      check(stock_icons==24&&stock_deaths==dead_rows*6,"Held/final scoreboard did not select the original alive/dead icons");
    }
  }
  // Expected strip geometry/colors are recorded by unchanged Vista46BF66.
  struct Strip {std::array<int,4> rectangle;std::array<std::uint32_t,4> color;};
  std::map<std::pair<unsigned,std::uint32_t>,std::vector<Strip>> quality_cases;
  std::ifstream quality_fixture(std::filesystem::path(__FILE__).parent_path()/"fixtures/vista_full_scoreboard_quality.tsv");
  check(bool(quality_fixture),"Original Vista connection fixture missing");
  while(std::getline(quality_fixture,line)) {
    if(line.empty()||line[0]=='#')continue;
    unsigned quality;std::uint32_t alpha;Strip strip;std::istringstream row(line);row>>quality>>alpha;
    for(auto &v:strip.rectangle)row>>v;for(auto &v:strip.color)row>>v;
    check(bool(row),"Invalid original Vista connection strip");quality_cases[{quality,alpha}].push_back(strip);
  }
  auto font=load_retail_font(map.parent_path()/"fonts/conduit-13");
  FullScoreboardPainter painter(font,0,1);
  FullScoreboardPaintFrame frame;frame.clip={0,0,480,640};
  for(const auto &[key,strips]:quality_cases) {
    FullScoreboardLayout layout;layout.status=FullScoreboardStatus::Ready;layout.rows.resize(1);
    auto &connection=layout.rows[0].widgets[5];connection.valid=true;
    connection.rectangle={20,100,39,106};connection.foreground={std::bit_cast<float>(key.second),1,1,1};
    connection.text.assign(1,char16_t(key.first));
    const auto paint=painter.draw(layout,*cache.original_data()->full_scoreboard_paint_constants,frame);
    check(!paint.connection_quality_unavailable&&paint.draw.vertices.size()==(strips.size()+1)*6,"Vista quality strip count differs");
    for(unsigned i=0;i<strips.size();++i) {
      const auto &s=strips[i];
      const auto &a=paint.draw.vertices[(i+1)*6],&b=paint.draw.vertices[(i+1)*6+4];
      check(a.position.x==s.rectangle[1]&&a.position.y==s.rectangle[0]&&b.position.x==s.rectangle[3]&&b.position.y==s.rectangle[2],"Vista quality strip bounds differ from original instructions");
      const std::array<std::uint32_t,4> color{std::bit_cast<std::uint32_t>(a.position.z),std::bit_cast<std::uint32_t>(a.color.x),std::bit_cast<std::uint32_t>(a.color.y),std::bit_cast<std::uint32_t>(a.color.z)};
      check(color==s.color,"Vista quality strip color differs from original instructions");
    }
  }
  check(quality_cases.size()==27,"Original Vista quality coverage incomplete");
  std::cout<<bindings<<" actual Vista bitmap bindings; "<<cases<<" pause/pre-carnage layouts across all local views and both aspects pass\n";
  std::cout<<quality_cases.size()<<" connection cases match unchanged Vista instructions\n";
  std::cout<<"LIMIT CPU submission evidence; production SDL/Vulkan routes are checked separately\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
