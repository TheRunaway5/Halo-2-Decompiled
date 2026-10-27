#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_scoreboard.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/frontend_font_sampling.hpp"
#include <algorithm>
#include "halo2/source_camera.hpp"
#include "halo2/collision.hpp"
#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw FormatError(message);}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {
  return Bytes(memory.view(address,4)).u32(0);
}
struct Files {
  std::filesystem::path path;
  Files() {
    const auto *scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Source phase regression requires owned scratch");
    path=std::filesystem::path(scratch)/"scoreboard-phase-world";std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;
  check(argc==2 || argc==3,"Supply MAP [TU15_XBE]; a prepared MCC map owns its imported TU1.5 metadata");
  if(!std::filesystem::exists(argv[1]) || (argc==3&&!std::filesystem::exists(argv[2])))return 77;
  Files files;MultiplayerOptions options;options.map=argv[1];if(argc==3)options.gameplay_xbe=argv[2];
  options.source_object_world=true;options.source_object_savegame_directory=files.path;options.local_players=2;
  options.variant={"Scoreboard source phase",0,{{89,5},{17,5},{2,0},{10,0}}};options.starting_weapon=5;
  options.session_random_seed=0x12345678u;
  MultiplayerPreparedWorld prepared(options);prepared.bind_source_local_views(prepared.local_slots);
  auto &match=prepared.match;auto &objects=*prepared.source_objects;auto &world=objects.tu15_world();
  const auto &memory=world.memory();const auto original=prepared.cache->original_data();
  check(match.source_engine_owned() && prepared.local_slots.size()==2,"Source phase test selected another match owner");
  Mesh mesh;MultiplayerScoreboard board(*prepared.cache,gameplay_font_directory(*prepared.cache),mesh,options.variant);
  check(board.available(),"Original source scoreboard constructor unavailable");
  if(prepared.cache->is_mcc()) {
    const auto source=load_gameplay_font_source(gameplay_font_directory(*prepared.cache));
    check(source.domain==GameplayFontDomain::Mcc,"MCC scoreboard selected a non-MCC runtime font owner");
    auto admitted_font=load_gameplay_font(source,6);
    apply_vista_prompt_sampling(admitted_font,font_prompt_asset_directory(source.directory,{}),6);
    const auto material=std::find_if(mesh.materials.begin(),mesh.materials.end(),
        [](const auto &row){return row.name=="scoreboard/font6";});
    check(material!=mesh.materials.end()&&material->diffuse.width==admitted_font.atlas.width&&
        material->diffuse.height==admitted_font.atlas.height&&material->diffuse.format==admitted_font.atlas.format&&
        material->diffuse.pixels==admitted_font.atlas.pixels,
        "Actual scoreboard material did not retain the admitted high-definition MCC font6/prompt pixels");
  }
  const auto ids=prepared.local_slots;const auto shooter=ids[0],victim=ids[1];
  const auto rows=word(memory,word(memory,0x4f54f4u)+0x44u),locals=word(memory,0x4f54f0u);
  for(unsigned view=0;view<ids.size();++view) {
    const auto player=match.player(ids[view]).source_datum,row=rows+ids[view]*0x21cu;
    check(word(memory,row+0x24u)==view && word(memory,locals+0x1cu+view*4u)==player,
        "Actual151D30 logical input-user/reverse table binding missing");
    check(Bytes(memory.view(row+0x28u,2)).u16(0)==view && word(memory,locals+0xcu+view*4u)==player,
        "Actual151C90 local view binding differs from its independent input user");
  }
  std::array<Controls,multiplayer_capacity> controls{};unsigned compared{},nonzero{},rising{},falling{};
  std::array<unsigned,2> prior{};bool automatic{},restored{},dead{};std::uint32_t old_victim=null_datum;
  const auto compare=[&] {
    const auto state=match.state();const FullScoreboardClock clock{30,tick_seconds,std::int32_t(state.tick)};
    for(unsigned view=0;view<ids.size();++view) {
      const auto &player=state.players[ids[view]];
      check(player.source_status && player.source_status->scoreboard_gate,
          "Completed source frame omitted its actual15E6BC input capture");
      const auto &gate=*player.source_status->scoreboard_gate;
      check(std::uint32_t(gate.tick)+1u==state.tick,"Scoreboard gate is stale or captured after the source clock increment");
      board.step(view,false,state,ids[view]);
      const auto raw=std::to_integer<unsigned>(memory.view(0x4f63c0u+view,1)[0]);
      const auto draw=board.draw(state,ids,view,1280,720);
      check(draw.normal_hud==full_scoreboard_normal_hud(true,std::int16_t(view),std::uint8_t(raw),clock,
          *original->full_scoreboard_constants),"Native automatic scoreboard/HUD differs from actual original counter");
      if(raw) {
        check(draw.layout.status==FullScoreboardStatus::Ready && !draw.paint.draw.vertices.empty(),
            "Actual original counter did not reach native roster/font paint");
        const auto alpha=full_scoreboard_display_alpha(std::uint8_t(raw),clock,
            *original->full_scoreboard_constants,full_scoreboard_host_power);
        check(std::bit_cast<std::uint32_t>(draw.layout.input_alpha)==std::bit_cast<std::uint32_t>(alpha),
            "Native scoreboard alpha differs from actual original source counter");
        ++nonzero;
      } else check(draw.layout.rows.empty() && draw.paint.draw.vertices.empty(),
          "Native scoreboard remained after the original counter cleared");
      rising+=raw>prior[view];falling+=raw<prior[view];prior[view]=raw;++compared;
      if(view==1) {
        automatic|=player.source_unit==null_datum && !draw.normal_hud;
        restored|=automatic && player.source_unit!=null_datum && player.source_unit!=old_victim && raw==0 && draw.normal_hud;
      }
    }
  };
  const auto step=[&]{match.step(controls);compare();};
  for(unsigned tick=0;tick<4;++tick)step();
  check(match.player(shooter).source_current_object!=null_datum && match.player(victim).source_current_object!=null_datum,
      "Canonical source factory did not birth both participants");
  CollisionWorld collision(prepared.content.collision);std::array<Spawn,2> selected{};
  float closest=std::numeric_limits<float>::infinity();
  for(const auto &a:prepared.cache->spawns())for(const auto &b:prepared.cache->spawns()) {
    const auto separation=length(b.position-a.position);if(separation<2 || separation>8 || separation>=closest)continue;
    const auto eye=a.position+Vec3{0,0,prepared.content.player.camera_height};
    const auto delta=b.position+Vec3{0,0,prepared.content.player.camera_height}-eye;
    if(collision.raycast(eye,delta/length(delta),length(delta)))continue;
    selected={a,b};closest=separation;
  }
  check(std::isfinite(closest),"No clear authored spawn pair for actual source weapon death");
  // Same declared B95D0 setup as multiplayer_engine_combat_tu15_tests.
  // No vitality, damage, credit, death, timer or display result is supplied.
  world.set_object_position(match.player(shooter).source_current_object,selected[0].position);
  world.set_object_position(match.player(victim).source_current_object,selected[1].position);
  for(auto slot:ids){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
  const auto aim=[&] {
    const auto unit=match.player(shooter).source_current_object;if(unit==null_datum)return;
    const auto look=objects.player_look(match.player(shooter).source_datum,unit);if(!look)return;
    const auto &from=match.unit(shooter),&to=match.unit(victim);
    const auto eye=ordinary_unit_eye(from.body.position,from.definition.camera_height,from.definition.crouch_camera_height,from.crouch);
    const auto delta=to.body.position+Vec3{0,0,to.body.height*.5f}-eye;
    controls[shooter].yaw+=std::remainder(std::atan2(delta.y,delta.x)-look->x,Bytes(memory.view(0x44923cu,4)).f32(0));
    controls[shooter].pitch+=std::atan2(delta.z,std::hypot(delta.x,delta.y))-look->y;
  };
  while(match.state().tick<100){aim();step();}
  check(!match.unit(shooter).vitality.dead && !match.unit(victim).vitality.dead,"Warmup lost a source-born participant");
  old_victim=match.player(victim).source_current_object;const auto kills=match.player(shooter).kills,deaths=match.player(victim).deaths;
  controls[shooter].fire=true;
  for(unsigned tick=0;tick<240 && !dead;++tick){aim();step();dead=match.unit(victim).vitality.dead;}
  check(dead && match.player(shooter).kills==kills+1 && match.player(victim).deaths==deaths+1,
      "Ordinary source aim/fire did not produce a credited weapon death");
  controls[shooter].fire=false;
  for(unsigned tick=0;tick<240 && !restored;++tick)step();
  check(automatic && restored && nonzero && rising && falling,
      "Original raw scoreboard counter did not complete automatic death/fade/respawn restoration");
  // Unbind via the original owner and verify its reverse table; views stay
  // separate and are restored through the same ordinary local adapter.
  const auto victim_player=match.player(victim).source_datum;
  world.set_player_input_user(victim_player,-1);
  check(word(memory,rows+victim*0x21cu+0x24u)==null_datum && word(memory,locals+0x20u)==null_datum,
      "Original151D30 failed to unbind its logical user");
  prepared.bind_source_local_views(ids);
  check(word(memory,rows+victim*0x21cu+0x24u)==1 && word(memory,locals+0x20u)==victim_player,
      "Ordinary local adapter failed to restore independent input ownership");
  std::cout<<"PASS canonical TU1.5 source weapon death, actual151D30 input ownership, "<<compared
      <<" per-view original counter/HUD/alpha comparisons, automatic scoreboard, respawn and fade restoration; authored B95D0 positions and native controls are declared inputs\n";
  return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
