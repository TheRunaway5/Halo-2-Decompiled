#include "halo2/multiplayer_world.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_physics_entry_observation.hpp"
#include "halo2/source_teleporter.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address){return Bytes(memory.view(address,4)).u32(0);}
float distance_squared(Vec3 a,Vec3 b){return (a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z);}
struct Files {
  std::filesystem::path path;
  explicit Files(unsigned index) {
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");check(scratch,"Frame event tests need an owned scratch directory");
    path=std::filesystem::path(scratch)/("source-frame-events-"+std::to_string(index));
    std::filesystem::create_directories(path);
  }
  ~Files(){std::error_code error;std::filesystem::remove_all(path,error);}
};
}
int main(int argc,char** argv)try {
  std::cout<<std::unitbuf;
  if(argc!=3)throw FormatError("Usage: multiplayer_engine_frame_events_tu15_tests MCC_MAP REPORT");
  if(!std::filesystem::is_regular_file(argv[1]))return 77;
  Json report={{"passed",false},{"cases",Json::array()},
    {"scope","Canonical source-born players and ordinary full source frames. Explicit initial B95D0 position inputs exercise traversal/blocked-exit output delivery; this is not a played approach route, rendered/audio hardware proof or network acceptance."}};
  const auto save=[&]{std::ofstream out(argv[2]);out<<report.dump(2)<<'\n';check(bool(out),"Cannot retain source frame event report");};
  unsigned failures{};
  for(const bool remote:{false,true}) {
    Json row={{"name",remote?"no-local-view-recipient":"two-local-views"},{"passed",false},{"stage","construct"}};
    try {
      Files files(remote);MultiplayerOptions options;options.map=argv[1];options.local_players=2;
      options.variant={"Slayer",0,{{89,5}}};options.source_object_world=true;
      options.source_object_savegame_directory=files.path;
      MultiplayerPreparedWorld prepared(options);auto& match=prepared.match;auto& objects=*prepared.source_objects;
      auto& world=objects.tu15_world();auto& memory=world.memory();
      check(prepared.rules.requires_source_engine && objects.uses_scenario_player_spawns(prepared.cache->spawns()),
          "Frame event test selected the declared-placement adapter");
      const auto blocker=prepared.local_slots.at(0),entrant=prepared.local_slots.at(1);
      const auto blocker_unit=match.player(blocker).source_current_object,entrant_unit=match.player(entrant).source_current_object;
      check(blocker_unit!=null_datum && entrant_unit!=null_datum,"Canonical source factory did not birth both actual units");
      const auto binding=prepared.cache->load_first_person_binding(prepared.rules.weapon,2);
      match.configure_replicated_first_person(binding);
      const std::array locals{blocker,entrant};match.configure_local_views(std::span(locals).first(remote?1:2),binding);
      const auto pool=world.source_player_pool(),records=word(memory,pool+0x44u),stride=word(memory,pool+0x24u);
      if(remote)check(Bytes(memory.view(records+entrant*stride+0x28u,2)).u16(0)==0xffffu,
          "Remote notice case still owns a local source viewport");
      SourceTeleporterBank pairs;source_teleporter_initialize(pairs,prepared.content.netgame_flags);
      check(pairs.count>0,"Frame event map has no authored teleporter pair");
      const auto& entrance=prepared.content.netgame_flags.at(unsigned(pairs.pairs[0].entrance));
      const auto& exit=prepared.content.netgame_flags.at(unsigned(pairs.pairs[0].exit));
      row["stage"]="declared-position-input";
      // These are caller-owned input positions through the complete original
      // mover, after genuine canonical birth. No contact, event, admission,
      // source flag, query result or completed teleporter pose is supplied.
      world.set_object_position(blocker_unit,entrance.position);
      world.set_object_position(entrant_unit,entrance.position);
      unsigned frames{},phase_calls{},blocked_stores{},message_submissions{},preserved_callers{};
      std::uint32_t engine_stack_end{},parent_stack_begin{};
      std::vector<std::byte> parent_stack;
      std::vector<std::uint32_t> phases;std::vector<std::uint32_t> event_players;
      std::vector<std::string> sounds;
      SourcePhysicsEntryObserverBinding entries(memory,[&](bool before,auto entry,const auto& call) {
        if(!before)return;
        if(entry==0x15eba0u)engine_stack_end=call.gp[4]+4u;
        if(entry==0x15eba0u || entry==0x13b6f0u)phases.push_back(entry);
      });
      objects.observe_physics_stores([&](auto pc,const auto& store) {
        // Whole1A6830's first stack store proves actual phase execution.
        //1A6C07 completes event+8, after event22 and its duplicate player
        // field were already stored. Observe the original outputs directly.
        if(pc==0x1a6833u)phases.push_back(0x1a6830u);
        if(pc==0x15ebeeu) {
          parent_stack_begin=std::uint32_t(store.address)+4u;
          check(engine_stack_end>=parent_stack_begin && engine_stack_end-parent_stack_begin<0x10000u,
              "Original shared-frame caller stack extent is invalid");
          const auto bytes=memory.view(parent_stack_begin,engine_stack_end-parent_stack_begin);
          parent_stack.assign(bytes.begin(),bytes.end());
        }
        if(pc==0x15ebf9u) {
          check(!parent_stack.empty() && std::equal(parent_stack.begin(),parent_stack.end(),
              memory.view(parent_stack_begin,parent_stack.size()).begin()),
              "Output decoding or input rebasing overwrote the still-active original caller");
          ++preserved_callers;
        }
        if(pc==0x1a6c07u) {
          const auto player=Bytes(store.written).u32(0);
          check(word(memory,store.address-4u)==22u && word(memory,store.address+4u)==player,
              "Observed original event22 construction lost its actual player");
          event_players.push_back(player);
        }
        if(pc==0x1a67fcu && store.written.size()==1 && (std::to_integer<unsigned>(store.written[0])&4u))++blocked_stores;
        if(pc==0x25ab41u && store.written.size()==1 && store.written[0]==std::byte{1})++message_submissions;
      });
      objects.observe_sound_submissions([&](bool before,std::uint32_t sound,auto,const auto&,const auto&,auto,auto) {
        if(before)if(const auto* tag=prepared.cache->find_tag(TagHandle{sound}))sounds.push_back(tag->name);
      });
      std::array<Controls,multiplayer_capacity> controls{};
      for(const auto slot:prepared.local_slots){controls[slot].yaw=match.unit(slot).yaw;controls[slot].pitch=match.unit(slot).pitch;}
      const auto step=[&] {
        phases.clear();const auto before=match.state().tick;match.step(controls);++frames;
        check(phases==std::vector<std::uint32_t>{0x15eba0u,0x1a6830u,0x13b6f0u},
            "Ordinary frame duplicated/moved/omitted the actual teleporter phase");
        ++phase_calls;
        const auto state=match.state();check(state.tick==before+1 && state.tick==word(memory,word(memory,0x51df54u)+8u),
            "Event delivery changed the source/native clock owner");
      };
      row["stage"]="original-traversal-and-blocked-output";
      for(unsigned n=0;n<8;++n)step();
      check(match.player(blocker).source_teleport_serial==1 && match.player(entrant).source_teleport_serial==0,
          "Full source frame lost its actual traversal/blocked result");
      check(event_players==std::vector{match.player(blocker).source_datum},"Native activation does not match the actual original event22");
      const auto motion=objects.biped_motion(blocker_unit);const auto heading=std::atan2(motion.forward.y,motion.forward.x);
      check(distance_squared(motion.position,exit.position)<distance_squared(motion.position,entrance.position),
          "Original full-frame traversal did not reach the authored exit");
      check(std::abs(std::remainder(match.state().players[blocker].yaw-heading,6.28318530718f))<.0001f,
          "Unchanged native input restored the pre-teleport heading");
      const auto sound=match.player(blocker).source_teleport_sound;
      const auto* tag=prepared.cache->find_tag(TagHandle{sound});
      check(tag && tag->name=="sound\\game_sfx\\multiplayer\\teleporter_activate" &&
          std::ranges::find(sounds,tag->name)!=sounds.end(),"Native audio mirror differs from actual original sound selection/submission");
      check(blocked_stores>0 && match.player(blocker).source_messages==SourceHudMessages{},
          "Blocked source request was absent or its recipient leaked");
      const auto& notices=match.player(entrant).source_messages;unsigned count{};
      const auto hud=word(memory,0x50eeccu);
      for(const auto& notice:notices.rows)if(notice[0x82]!=std::byte{}) {
        ++count;
        if(!remote) {
          bool found{};
          for(unsigned n=0;n<4;++n) {
            const auto original=memory.view(hud+0x4e0u+n*0x88u,0x88u);
            found|=std::equal(notice.begin(),notice.begin()+0x83,original.begin()) &&
                std::equal(notice.begin()+0x84,notice.end(),original.begin()+0x84);
          }
          check(found,"Native blocked literal/tick/flags differ from the actual source local HUD bank");
        }
      }
      check(count>0 && (remote || message_submissions>0),"Actual blocked recipient lost its message output");
      row["blocked_notice_rows"]=count;row["original_local_message_submissions"]=message_submissions;
      row["stage"]="ordinary-blocker-motion-and-release";
      controls[blocker].forward=1;
      for(unsigned n=0;n<120 && !match.player(entrant).source_teleport_serial;++n)step();
      controls[blocker].forward=0;
      check(match.player(entrant).source_teleport_serial==1 &&
          event_players==std::vector{match.player(blocker).source_datum,match.player(entrant).source_datum},
          "Waiting participant did not consume its actual original event after the blocker walked clear");
      const auto entrant_heading=match.state().players[entrant].yaw;
      for(unsigned n=0;n<4;++n)step();
      check(match.player(blocker).source_teleport_serial==1 && match.player(entrant).source_teleport_serial==1 &&
          match.player(entrant).source_teleport_sound==sound &&
          std::abs(std::remainder(match.state().players[entrant].yaw-entrant_heading,6.28318530718f))<.0001f,
          "Original cooldown or unchanged input duplicated/reversed the completed teleport");
      check(preserved_callers==frames,"Original caller stack preservation was not observed for every frame");
      row["frames"]=frames;row["phase_calls"]=phase_calls;row["preserved_callers"]=preserved_callers;
      row["actual_event_players"]=event_players;
      row["blocked_stores"]=blocked_stores;row["actual_sound_submissions"]=sounds;row["passed"]=true;row["stage"]="complete";
    }catch(const std::exception& error){row["error"]=error.what();++failures;}
    std::cout<<row.dump()<<'\n';report["cases"].push_back(std::move(row));save();
  }
  report["passed"]=failures==0;save();return failures?1:0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
