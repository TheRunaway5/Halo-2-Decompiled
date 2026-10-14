#include "halo2/content.hpp"
#include "halo2/frontend_ui_assets.hpp"
#include "halo2/hud.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/ui_dds.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
using namespace halo2;
namespace {
void check(bool ok,const std::string &message) {if(!ok)throw std::runtime_error(message);}
constexpr std::string_view prefix="ui\\hud\\bitmaps\\new_hud\\crosshairs\\";
Texture read(const std::filesystem::path &path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);
  check(bool(file),"Missing Vista crosshair DDS");
  std::vector<std::byte> data(std::size_t(file.tellg()));file.seekg(0);
  check(bool(file.read(reinterpret_cast<char*>(data.data()),data.size())),"Truncated Vista crosshair DDS");
  return read_ui_dds_texture(Bytes(data));
}
}
int main(int argc,char **argv) {
  try {
    if(argc!=3)return 2;
    const auto maps=std::filesystem::path(argv[1]),vista=std::filesystem::path(argv[2]);
    if(!std::filesystem::exists(maps/"lockout.map") || !std::filesystem::exists(vista/"hud.tsv"))return 77;
    std::map<std::pair<std::string,unsigned>,Texture> expected;
    std::ifstream manifest(vista/"hud.tsv");std::string line;std::getline(manifest,line);
    while(std::getline(manifest,line)) {
      std::istringstream row(line);std::string name,path;unsigned frame,count,width,height;
      check(bool(std::getline(row,name,'\t') && row>>frame>>count>>width>>height>>path),"Invalid HUD manifest row");
      if(name.starts_with(prefix))expected.emplace(std::pair{name,frame},read(vista/path));
    }
    check(expected.size()==98,"Vista pack does not contain every authored crosshair page");
    Cache cache(maps/"shared.map");
    const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag) {
      return tag.group=="bipd" && tag.name=="objects\\characters\\masterchief\\masterchief_mp";
    });
    check(biped!=cache.tags().end(),"Missing original multiplayer biped");
    MultiplayerState state;
    for(auto &p:state.players) {p.connected=true;p.loaded=60;p.reserve=120;p.health=p.shield=1;
      p.secondary_weapon.datum=1;p.secondary_weapon.loaded=60;p.secondary_weapon.reserve=120;
      p.weapon_trigger.target_position=std::array{10.f,0.f,0.f};
      p.secondary_weapon.trigger.target_position=p.weapon_trigger.target_position;}
    unsigned weapons=0,widgets=0,vertices=0,bindings=0;
    std::set<std::pair<std::string,unsigned>> selected;
    for(const auto &tag:cache.tags()) {
      if(tag.group!="weap" || !tag.address || tag.size<0x2c8)continue;
      const auto metadata=cache.load_source_tag_metadata(tag.datum);
      if(Bytes(metadata.bytes_at(tag.address,tag.size)).u32(0x2b4)==null_datum)continue;
      auto content=cache.load_hud(biped->datum,tag.datum);
      // This fixture isolates reticle materials; action-message fonts/buttons
      // have their own additional materials in a selected PC artwork pack.
      content.action_messages.enabled=false;
      std::erase_if(content.widgets,[&](const auto &w) {
        return w.is_text || w.bitmap==null_datum || !content.images.at(w.bitmap).name.starts_with(prefix);
      });
      if(content.widgets.empty())continue;
      ++weapons;
      for(const auto &widget:content.widgets) {
        auto isolated=content;isolated.widgets={widget};
        // Exercise every authored row's page selection independently of its
        // gameplay visibility producer (weak-spot/lock-on/dual-hand states).
        isolated.widgets[0].yes={1,0,0,0};isolated.widgets[0].no={};
        for(unsigned hand=0;hand<=(widget.weapon_type==1?1u:0u);++hand) {
          Mesh old_mesh,hd_mesh;
          MultiplayerHud original(isolated,maps/"fonts",old_mesh,4,"Slayer",{}, {},hand);
          MultiplayerHud hd(isolated,maps/"fonts",hd_mesh,4,"Slayer",{},vista,hand);
          check(old_mesh.materials.size()==hd_mesh.materials.size(),"Vista changed reticle material topology");
          const auto &image=content.images.at(widget.bitmap);
          for(unsigned view=0;view<4;++view)for(unsigned page=0;page<image.images.size();++page) {
            const auto &actual=hd_mesh.materials.at(view*image.images.size()+page).diffuse;
            const auto &want=expected.at({image.name,page});
            check(actual.width==want.width && actual.height==want.height && actual.format==want.format &&
                  actual.pixels==want.pixels,"Crosshair material still uses Xbox pixels: "+tag.name);
            ++bindings;
          }
          for(unsigned width:{640u,1280u})for(unsigned players=1;players<=4;++players)for(unsigned view=0;view<players;++view) {
            const SourceHudAimCamera camera{{},{1,0,0},{0,0,1},1.f,false};
            const auto a=original.draw(state,view,view,players,width,width==640?480:720,25,14400,{}, {},camera);
            const auto b=hd.draw(state,view,view,players,width,width==640?480:720,25,14400,{}, {},camera);
            check(a.size()==b.size() && !b.empty(),"Missing authored crosshair row: "+tag.name);
            for(unsigned n=0;n<a.size();++n) {
              check(a[n].position.x==b[n].position.x && a[n].position.y==b[n].position.y &&
                    a[n].uv.x==b[n].uv.x && a[n].uv.y==b[n].uv.y && a[n].material==b[n].material,
                    "Vista crosshair changed the current full/split/dual-hand layout: "+tag.name);
              ++vertices;
            }
            const auto mode=hud_viewport(players,view,width!=640).mode;
            const auto page=image.sequences.at(widget.sequence[mode]).at(0).image;
            selected.emplace(image.name,page);
            check(hd_mesh.materials.at(b[0].material).diffuse.pixels==expected.at({image.name,page}).pixels,
                  "Wrong Vista full/split crosshair variant selected: "+tag.name);
          }
        }
        ++widgets;
      }
    }
    check(weapons>=25 && widgets>=60 && selected.size()>=50,"Crosshair regression missed authored weapon/vehicle variants");
    // An incomplete selected Vista pack must fail instead of silently using
    // the original Xbox crosshair bank.
    const auto smg=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto &tag) {
      return tag.group=="weap" && tag.name=="objects\\weapons\\rifle\\smg\\smg";
    });
    check(smg!=cache.tags().end(),"Missing original SMG");
    auto content=cache.load_hud(biped->datum,smg->datum);
    const auto missing=std::filesystem::temp_directory_path()/("halo2-crosshair-incomplete-"+
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(missing);
    {std::ofstream file(missing/"hud.tsv");file<<"H2UI_SAMPLING_PACK_V2\n";}
    bool rejected=false;
    try {Mesh mesh;MultiplayerHud hud(content,maps/"fonts",mesh,1,"Slayer",{},missing);}
    catch(const FormatError &e){rejected=std::string(e.what()).find("Missing Vista crosshair artwork")!=std::string::npos;}
    std::filesystem::remove_all(missing);check(rejected,"Incomplete Vista pack silently used Xbox crosshairs");
    std::cout<<weapons<<" weapons, "<<widgets<<" authored crosshair rows, "<<selected.size()<<" selected pages, "
             <<bindings<<" Vista material bindings and "<<vertices<<" unchanged full/split/dual-hand vertices\n";
    return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
