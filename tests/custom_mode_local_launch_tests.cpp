#include "halo2/frontend.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/multiplayer_network.hpp"
#include "halo2/multiplayer_outcome.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/native_vista_flow.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void need(bool condition,const std::string &message) {
  if(!condition)throw FormatError(message);
}
unsigned value_row(const MenuState &menu,int value) {
  const auto &rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,value,&MenuEntry::value);
  need(found!=rows.end(),"Custom launch lost its authored menu row");
  return unsigned(found-rows.begin());
}
unsigned action_row(const MenuState &menu,int action) {
  const auto &rows=menu.frame().entries;
  const auto found=std::ranges::find(rows,action,&MenuEntry::destination);
  need(found!=rows.end(),"Custom launch lost its menu action");
  return unsigned(found-rows.begin());
}
void choose(MenuState &menu,unsigned row,unsigned screen) {
  menu.select(row);menu.accept();
  need(menu.frame().screen==screen,"Custom launch missed menu screen "+std::to_string(screen));
}
FrontendLaunch selected(const FrontendContent &content,unsigned custom,unsigned players,bool lan) {
  MenuState menu(content);menu.accept();choose(menu,0,167);
  menu.text("Mode Pilot 1");menu.commit_text();menu.back();
  need(menu.frame().screen==6,"Custom primary profile did not reach the main menu");
  choose(menu,lan?3:2,30);choose(menu,0,14);
  need(menu.controller_slot(100,true)==0,"Custom host controller did not bind");
  for(unsigned player=1;player<players;++player) {
    need(menu.controller_slot(100+player,true)==player && menu.frame().screen==10,
        "Custom local controller could not join before LAN admission");
    choose(menu,0,167);menu.text("Mode Pilot "+std::to_string(player+1));menu.commit_text();menu.back();
    need(menu.frame().screen==14 && menu.local_players().size()==player+1,
        "Custom local profile did not return to its lobby");
  }
  const auto &variant=content.bundled_variants.at(custom);
  choose(menu,action_row(menu,23),23);choose(menu,action_row(menu,207),207);
  choose(menu,value_row(menu,int(variant.engine)),208);
  choose(menu,value_row(menu,int(content.variants.size()+custom)),14);
  if(bomb_defusal_variant(variant)) {
    // Commit the host's actual team through the normal lobby editor. Other
    // local profiles retain their selected default0; no team is synthesized.
    menu.auxiliary();menu.navigate(1,0);menu.accept();
    need(menu.lobby_view()->player_team==1,"Defusal host did not commit its playing team");
  }
  menu.select(action_row(menu,-4));menu.accept();
  need(menu.launch.has_value(),variant.name+": normal Start refused the custom game: "+menu.frame().description);
  need(menu.launch->local_players==players && menu.launch->selected_profiles.size()==players &&
       menu.launch->host_port==(lan?2002u:0u),"Custom Start changed its selected local roster or mode");
  return *menu.launch;
}
MultiplayerOptions requested(const FrontendLaunch &launch,const std::filesystem::path &maps) {
  MultiplayerOptions options;options.map=maps/launch.map;options.shared_maps=maps;
  options.variant=launch.variant;options.local_players=launch.local_players;options.host_port=launch.host_port;
  options.selected_profiles=launch.selected_profiles;options.selected_teams=launch.selected_teams;
  options.input_instances=launch.input_instances;options.source_object_world=true;options.mute=true;
  return options;
}
void local_views(MultiplayerPreparedWorld &world) {
  std::vector<FirstPersonBindingDefinition> definitions;
  for(const auto &[datum,weapon]:world.content.weapons)if(!weapon.first_person_models.empty())
    for(unsigned representation:source_session_player_representations(world.content,2))
      if(representation<world.content.player_representations.size() &&
          world.content.player_representations[representation].biped.datum!=null_datum) {
        auto binding=world.cache->load_first_person_binding(datum,representation);
        if(binding.record_present && binding.weapon_model!=null_datum && binding.graph!=null_datum)
          definitions.push_back(std::move(binding));
      }
  world.match.configure_local_views(world.local_slots,definitions);
  for(unsigned view=0;view<world.local_slots.size();++view)
    need(world.match.first_person_view(view).unit==world.match.player(world.local_slots[view]).source_current_object,
         "Custom local presentation bound another player unit");
}
Json exercise(const FrontendContent &content,unsigned custom,unsigned players,bool lan,
              const std::filesystem::path &maps) {
  const auto launch=selected(content,custom,players,lan);
  const auto &variant=launch.variant;const auto custom_kind=custom_slayer(variant);
  const bool defusal=bomb_defusal_variant(variant);
  need(!NativeVistaOfflineFlow::supports(launch),"Native custom mode entered the stock Vista lifecycle");
  auto options=requested(launch,maps);
  const auto expected=source_game_variant(variant);
  select_multiplayer_mode_authority(options);
  need(!options.source_object_world && source_game_variant(options.variant).bytes==expected.bytes &&
       options.variant.values==variant.values,"Mode routing changed the defining custom rules");
  MultiplayerPreparedWorld world(options);
  need(!world.source_objects && !world.source_savegames && !world.match.source_engine_owned() &&
       !world.rules.requires_source_engine && world.rules.custom==custom_kind && bool(world.rules.defusal)==defusal,
       "MCC custom launch did not retain its intended native custom authority");
  need(world.matches(options) && world.local_slots.size()==players && world.match.session_configuration() &&
       world.match.session_configuration()->values==variant.values &&
       source_game_variant(*world.match.session_configuration()).bytes==expected.bytes,
       "Custom prepared world lost its exact mode marker, settings or roster");
  local_views(world);
  std::array<Controls,multiplayer_capacity> controls{};
  for(unsigned tick=0;tick<(defusal?161u:3u);++tick)world.match.step(controls);
  auto state=world.match.state();
  for(unsigned view=0;view<world.local_slots.size();++view) {
    const auto slot=world.local_slots[view];
    need(state.players[slot].connected && !state.players[slot].dead,
        "Custom local participant did not remain alive in its native authority");
    need(world.match.first_person_view(view).unit==world.match.player(slot).source_current_object,
        "Custom local presentation lost its current unit after an ordinary mode step");
  }
  if(custom_kind==CustomSlayer::infection) {
    unsigned zombies{};
    for(auto slot:world.local_slots) {
      need(state.players[slot].source_configuration.has_value(),"Infection omitted its actual faction appearance");
      zombies+=(*state.players[slot].source_configuration)[0x40]==6;
    }
    need(zombies==1,"Infection did not retain exactly one initial zombie");
  }
  if(defusal)need(state.defusal && state.defusal_rules && state.defusal->phase==BombDefusalPhase::live &&
      state.defusal_rules->round_ticks==150u*30u && state.defusal_rules->fuse_ticks==45u*30u &&
      state.defusal_rules->plant_ticks==4u*30u && state.defusal_rules->defuse_ticks==6u*30u,
      "Defusal changed its defining timings or failed to start both committed sides");
  Json result={{"name",variant.name},{"local_players",players},{"lan",lan},{"exact_settings",true},
      {"native_custom_authority",true},{"local_presentation_views",players},{"ticks",state.tick}};
  if(lan) {
    const auto fingerprint=custom_slayer_fingerprint(multiplayer_fingerprint(options.map,maps,0,
        world.network_starting_weapon_identity()),variant);
    LanHost host(world.match,0,fingerprint,variant,options.map.stem().string());
    const auto description=query_lan_session(host.join_address());
    need(description.map==options.map.stem().string() &&
         description.fingerprint==fingerprint && description.variant.values==variant.values &&
         source_game_variant(description.variant).bytes==expected.bytes,
         "Authenticated custom LAN discovery changed the defining rules");
    const std::array<std::uint8_t,9> appearance{};
    const std::array<SourcePlayerConfiguration,1> profiles{
        native_selected_player_configuration("Custom Peer",appearance,defusal?1:0)};
    const std::array<std::int8_t,1> playing_team{1};
    const auto teams=defusal?std::span<const std::int8_t>(playing_team):std::span<const std::int8_t>{};
    LanClient client(host.join_address(),fingerprint,1,teams,profiles);
    std::array<Controls,1> input{};
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
    while(!client.ready()) {
      client.send(input);host.receive(controls);host.broadcast();client.receive();
      need(client.error().empty(),"Custom LAN peer failed admission: "+client.error());
      need(std::chrono::steady_clock::now()<deadline,"Custom LAN peer admission timed out");
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    const auto &received=client.state();
    need(unsigned(std::ranges::count_if(received.players,[](const auto &p){return p.connected;}))==players+1 &&
         received.defusal.has_value()==defusal && !received.source_body_poses_authoritative,
         "Custom LAN snapshot lost its roster or switched to the stock source-world contract");
    auto replica_options=options;replica_options.host_port=0;replica_options.join=host.join_address();
    replica_options.local_players=1;replica_options.selected_profiles.assign(profiles.begin(),profiles.end());
    replica_options.selected_teams.assign(teams.begin(),teams.end());replica_options.input_instances={-1};
    replica_options.variant=description.variant;replica_options.source_object_world=true;
    select_multiplayer_mode_authority(replica_options);
    MultiplayerPreparedWorld replica(replica_options);
    need(!replica.source_objects && replica.local_slots.empty() && replica.rules.custom==custom_kind &&
         bool(replica.rules.defusal)==defusal,"MCC custom peer prepared another authority or invented local units");
    result["authenticated_lan_roundtrip"]=true;result["peer_slot"]=client.slots()[0];
    result["authenticated_map"]=description.map;
    result["custom_client_prepared"]=true;
  }
  return result;
}
Json runtime_smoke(const FrontendContent &content,unsigned custom,const std::filesystem::path &maps,bool render) {
  const auto launch=selected(content,custom,2,false);
  auto options=requested(launch,maps);
  const auto expected=source_game_variant(launch.variant);
  const bool defusal=bomb_defusal_variant(launch.variant);
  unsigned loaded{},updates{},submitted{};std::array<unsigned,2> view_calls{};
  std::array<UiRect,2> rectangles{};
  Json defusal_startup=Json::array();
  const auto verify=[&](const MultiplayerMatch &match) {
    need(!match.source_engine_owned() && match.session_configuration() &&
         match.session_configuration()->values==launch.variant.values &&
         source_game_variant(*match.session_configuration()).bytes==expected.bytes &&
         custom_slayer(*match.session_configuration())==custom_slayer(launch.variant) &&
         bomb_defusal_variant(*match.session_configuration())==defusal,
         "Actual runtime changed the selected custom authority or defining settings");
    const auto state=match.state();
    need(unsigned(std::ranges::count_if(state.players,[](const auto &p){return p.connected;}))==2 &&
         state.defusal.has_value()==defusal && !state.source_body_poses_authoritative,
         "Actual runtime changed the two selected local participants or custom snapshot contract");
  };
  options.session_world_loaded=[&](const MultiplayerMatch &match,std::span<const unsigned> slots) {
    need(slots.size()==2,"Actual runtime did not insert both menu-selected local participants");
    verify(match);++loaded;
  };
  options.session_world_update=[&](const MultiplayerMatch &match) {verify(match);++updates;return false;};
  options.source_world_observer=[](const SourceRuntimeMemory &) {
    throw FormatError("Actual custom runtime constructed a stock source object world");
  };
  if(render) {
    options.frames=2;options.width=640;options.height=480;options.hidden=true;
    if(defusal) {
      const auto stock_objective=[](unsigned,std::uint32_t,std::span<const Vertex>) {
        throw FormatError("Custom Defusal entered the stock source objective presentation");
      };
      options.objective_waypoint_draw_observer=stock_objective;
      options.objective_goal_draw_observer=stock_objective;
    }
    options.frame_observer=[&](const MultiplayerFrameTiming &timing) {submitted+=timing.submitted;};
    options.view_rectangle_observer=[&](unsigned view,const UiRect &rect) {
      need(view<2 && rect.top>=0 && rect.left>=0 && rect.bottom<=480 && rect.right<=640 &&
           rect.bottom>rect.top && rect.right>rect.left,"Custom runtime submitted an invalid split-screen view");
      rectangles[view]=rect;++view_calls[view];
    };
    options.view_state_observer=[&](unsigned view,const MultiplayerState &state,const Camera &) {
      need(view<2 && state.defusal.has_value()==defusal && !state.source_body_poses_authoritative,
           "Rendered custom view received another mode's presentation contract");
      if(defusal) {
        const auto &bomb=*state.defusal;
        const Json diagnostic={{"view",view},{"tick",state.tick},{"has_rules",bool(state.defusal_rules)},
            {"sites",bomb.sites.size()},{"phase_id",unsigned(bomb.phase)},
            {"phase",bomb.phase==BombDefusalPhase::waiting?"waiting":
                bomb.phase==BombDefusalPhase::freeze?"freeze":bomb.phase==BombDefusalPhase::live?"live":"other"},
            {"round",bomb.round},{"remaining",bomb.remaining}};
        defusal_startup.push_back(diagnostic);
        need(state.defusal_rules && bomb.sites.size()==2,
            "Rendered Defusal view lost its custom site markers: "+diagnostic.dump());
        // Construction publishes authored sites before the first authority
        // tick. That tick admits both teams and starts the preparation phase;
        // two submitted frames need not have advanced that clock yet.
        const bool initial_wait=bomb.phase==BombDefusalPhase::waiting && state.tick==0 &&
            bomb.round==0 && bomb.remaining==0;
        need(initial_wait || bomb.phase==BombDefusalPhase::freeze || bomb.phase==BombDefusalPhase::live,
            "Rendered Defusal view has invalid startup state: "+diagnostic.dump());
      }
    };
  } else options.ticks=3;
  // Deliberately retain the MCC application's stock source-world request.
  // The public runtime must resolve the selected custom mode itself.
  MultiplayerRuntimeOutcome outcome;
  need(run_multiplayer(options,outcome)==0,"Actual custom multiplayer runtime returned an error");
  need(options.source_object_world && loaded==1 && outcome.authority==MultiplayerOutcomeAuthority::local &&
       outcome.reason==(render?MultiplayerExitReason::frame_limit:MultiplayerExitReason::headless_limit) &&
       outcome.state && unsigned(std::ranges::count_if(outcome.state->players,
           [](const auto &p){return p.connected;}))==2 && outcome.state->defusal.has_value()==defusal,
       "Actual custom runtime did not complete its bounded local run with the selected roster");
  if(render)need(submitted==2 && view_calls[0]==2 && view_calls[1]==2 &&
      (rectangles[0].top!=rectangles[1].top || rectangles[0].left!=rectangles[1].left),
      "Custom runtime did not submit both split-screen views for both frames");
  else need(updates==3 && outcome.state->tick==3,"Custom headless runtime did not advance its three actual ticks");
  return { {"name",launch.variant.name},{"local_players",2},{"source_world_requested",true},
      {"native_custom_authority",true},{"exact_settings",true},{"mode",render?"render":"headless"},
      {"ticks",outcome.state->tick},{"runtime_updates",updates},{"submitted_frames",submitted},
      {"view_submissions",view_calls},{"defusal_startup",defusal_startup},
      {"stock_objective_presentation",false},{"capture_files",0} };
}
}
int main(int argc,char **argv)try {
  need(argc==3 || (argc==4 && std::string_view(argv[3])=="--render"),"Supply MCC_MAPS REPORT [--render]");
  const bool render=argc==4;const std::filesystem::path maps=argv[1];
  if(!std::filesystem::is_regular_file(maps/"mainmenu.map"))return 77;
  need(std::getenv("HALO2_TEST_SCRATCH"),"Custom launch regression requires owned scratch");
  Cache cache(maps/"mainmenu.map");need(cache.is_mcc(),"Custom launch regression requires imported MCC assets");
  const auto content=cache.load_frontend();
  Json report={{"passed",false},{"cases",Json::array()},{"runtime_smokes",Json::array()},{"render",render},
      {"scope",render?
          "Three actual MCC two-player menu Starts through run_multiplayer, each submitting two split-screen frames with the selected custom authority and unchanged defining rules. No captures; full visual/audio/gameplay parity is separate.":
          "Actual MCC menu Starts, native custom-authority construction/steps/local views, authenticated LAN discovery/admission/snapshots and three actual headless run_multiplayer launches. Defining rules remain fixed; full gameplay/render/audio parity is separate."}};
  const auto save=[&]{std::ofstream(argv[2])<<report.dump(2)<<'\n';};save();unsigned tested{},smokes{};
  MultiplayerOptions stock;stock.source_object_world=true;select_multiplayer_mode_authority(stock);
  need(stock.source_object_world,"Custom routing changed the stock native-world default");
  for(unsigned custom=0;custom<content.bundled_variants.size();++custom) {
    const auto &variant=content.bundled_variants[custom];
    if(custom_slayer(variant)==CustomSlayer::none && !bomb_defusal_variant(variant))continue;
    if(!render)for(bool lan:{false,true})for(unsigned players:{2u,4u}) {
      try {report["cases"].push_back(exercise(content,custom,players,lan,maps));++tested;save();}
      catch(const std::exception &error) {
        report["cases"].push_back({{"name",variant.name},{"local_players",players},{"lan",lan},
            {"passed",false},{"error",error.what()}});save();
        throw FormatError(variant.name+" / "+(lan?"LAN":"Split-screen")+" / "+
            std::to_string(players)+" local: "+error.what());
      }
    }
    try {report["runtime_smokes"].push_back(runtime_smoke(content,custom,maps,render));++smokes;save();}
    catch(const std::exception &error) {
      report["runtime_smokes"].push_back({{"name",variant.name},{"mode",render?"render":"headless"},
          {"passed",false},{"error",error.what()}});save();
      throw FormatError(variant.name+" / actual "+(render?"rendered":"headless")+" runtime: "+error.what());
    }
  }
  need((render || tested==12) && smokes==3,
      "Custom launch regression did not cover all three modes in its actual runtime and requested local/LAN cases");
  report["passed"]=true;save();
  std::cout<<"PASS "<<tested<<" MCC custom local/LAN cases and "<<smokes<<" actual "
      <<(render?"two-frame split-screen":"three-tick headless")<<" runtime launches; no captures\n";return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
