#include "halo2/cache.hpp"
#include "halo2/content.hpp"
#include "halo2/multiplayer.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_teleporter.hpp"
#include "halo2/hud.hpp"
#include "halo2/gameplay_font_source.hpp"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
struct Files {
  std::filesystem::path path;
  Files(){auto name=(std::filesystem::temp_directory_path()/"halo2-teleporter-notice-XXXXXX").string();
    check(::mkdtemp(name.data()),"Cannot create owned notice test directory");path=name;}
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  if(argc!=2)return 2;
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Cache cache(argv[1]);check(cache.is_mcc(),"Blocked notice acceptance requires prepared MCC assets");
  const auto content=cache.load_content(0,true);
  SourceTeleporterBank pairs;source_teleporter_initialize(pairs,content.netgame_flags);
  check(pairs.count>0 && !cache.spawns().empty(),"Notice test needs an authored teleporter/start");
  auto start=cache.spawns().front();start.position=content.netgame_flags[unsigned(pairs.pairs[0].entrance)].position;
  start.facing=content.netgame_flags[unsigned(pairs.pairs[0].entrance)].facing;
  Files files;MultiplayerObjectRuntime world(cache,0,files.path);
  const UiVariant variant{"Slayer",0,{{89,5}}};auto rules=slayer_rules(variant);
  rules.weapon=cache.slayer_starting_weapon(rules.starting_weapon);
  world.configure_session(source_game_variant(variant),1,30,rules.random_seed);
  MultiplayerMatch match(content,0,std::span(&start,1),rules,{}, {},&world);
  const auto blocker=match.join("Actual exit occupant"),entrant=match.join("Actual waiting local player");
  const auto binding=cache.load_first_person_binding(rules.weapon,2);match.configure_replicated_first_person(binding);
  const std::array locals{blocker,entrant};match.configure_local_views(locals,binding);
  unsigned actual_submissions{};
  world.observe_physics_stores([&](auto pc,const auto &store) {
    if(pc==0x25ab41u && store.written.size()==1 && std::to_integer<unsigned>(store.written[0])==1)
      ++actual_submissions;
  });
  std::array<Controls,multiplayer_capacity> controls{};
  for(auto player:locals){controls[player].yaw=match.unit(player).yaw;controls[player].pitch=match.unit(player).pitch;}
  for(unsigned tick=0;tick<8;++tick)match.step(controls);
  check(match.player(blocker).source_teleport_serial==1 && match.player(entrant).source_teleport_serial==0,
      "Notice setup did not reach an actual occupied exit");
  auto &memory=world.memory();const auto word=[&](auto p){return Bytes(memory.view(p,4)).u32(0);};
  const auto pool=word(0x50eeccu),hud_tag=word(0x51df94u);nlohmann::json views=nlohmann::json::array();
  unsigned waiting_rows{};
  for(unsigned view=0;view<2;++view) {
    nlohmann::json rows=nlohmann::json::array();
    for(unsigned row=0;row<4;++row) {
      const Bytes record(memory.view(pool+view*0x4e0u+row*0x88u,0x88));
      if(record.slice(0x82,1).front()==std::byte{})continue;
      std::vector<std::uint16_t> text;for(unsigned at=4;at<0x82;at+=2){const auto c=record.u16(at);if(!c)break;text.push_back(c);}
      rows.push_back({{"tick",record.u32(0)},{"utf16",text}});if(view==1 && !text.empty())++waiting_rows;
    }
    views.push_back(rows);
  }
  std::cout<<nlohmann::json{{"actual_submissions",actual_submissions},{"source_hud",hud_tag},
      {"message_pool",pool},{"view_rows",views}}.dump()<<'\n';
  check(actual_submissions>0 && waiting_rows>0,"Actual original blocked notice did not reach its retained local HUD queue");
  const auto snapshot=match.state();
  SourceHudMessages original;
  for(unsigned row=0;row<4;++row)memory.read(pool+0x4e0u+row*0x88u,original.rows[row]);
  unsigned matched{};
  for(const auto &row:snapshot.players[entrant].source_messages.rows)if(row[0x82]!=std::byte{}) {
    // The original pool also contains the independent round-start message.
    // Compare the delivered literal/tick/flags, excluding its source-global
    // order byte shared with producers outside this transport's scope.
    bool found{};for(const auto &source:original.rows)
      found|=std::equal(row.begin(),row.begin()+0x83,source.begin()) &&
          std::equal(row.begin()+0x84,row.end(),source.begin()+0x84);
    check(found,"Native blocked literal/tick differs from an actual original local message row");++matched;
  }
  check(matched>0,"Native recipient has no actual blocked literal");
  check(snapshot.players[blocker].source_messages==SourceHudMessages{},"Blocked notice leaked to the exit occupant");
  auto authored=cache.load_hud(content.player.datum,0xffffffffu);
  check(authored.messages.enabled,"Native unit HUD does not own its literal messages");
  authored.widgets.clear();authored.action_messages.enabled=false;authored.sensor={};
  Mesh mesh;MultiplayerHud hud(authored,gameplay_font_directory(cache),mesh,2);
  const auto waiting=hud.draw(snapshot,entrant,1,2,640,480,0,0);
  const auto absent=hud.draw(snapshot,blocker,0,2,640,480,0,0);
  check(!waiting.empty() && absent.empty(),"Actual blocked text did not reach only its native HUD viewport");
  const auto viewport=hud_viewport(2,1);
  for(const auto &vertex:waiting)check(mesh.materials.at(vertex.material).name=="action-message/font5" &&
      vertex.position.x>=viewport.safe.left && vertex.position.x<=viewport.safe.right &&
      vertex.position.y>=viewport.safe.top && vertex.position.y<=viewport.safe.bottom,
      "Blocked text lost its original split font or escaped its recipient viewport");
  auto expired=snapshot;expired.tick+=300;
  check(hud.draw(expired,entrant,1,2,640,480,0,0).empty(),"Expired blocked text still draws");
  std::cout<<"PASS actual authored blocked text/tick/flags, native split HUD glyphs and expiry\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
