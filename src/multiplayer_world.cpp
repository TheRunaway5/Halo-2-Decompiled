#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/online_match_session.hpp"
#include <algorithm>
#include <cstdlib>

namespace halo2 {
namespace {
TagSystem world_tags(std::shared_ptr<const Cache> cache, std::size_t bsp) {
  TagSystem tags(std::move(cache));
  tags.load_bsp(bsp);
  return tags;
}
LevelContent world_content(const Cache &cache,const MultiplayerOptions &options,
    std::optional<ScriptProgram> &script_input) {
  auto content=cache.load_content(options.bsp,true,source_game_variant(options.variant));
  if(script_input) {
    if(options.online_match || options.host_port || !options.join.empty() || options.dedicated)
      throw FormatError("Declared script inputs require an offline acceptance world");
    content.scripts=std::move(*script_input);
  }
  return content;
}
SlayerRules world_rules(const Cache& cache,const MultiplayerOptions& options) {
  auto variant=options.variant;
  if(options.starting_weapon)variant.values.set(0x59,int(options.starting_weapon));
  auto rules=slayer_rules(variant,true);
  if(!options.selected_profiles.empty() && options.selected_profiles.size()!=options.local_players)
    throw FormatError("Selected profiles must provide one complete record per local player");
  if((!options.selected_teams.empty() || rules.team_mode) &&
      (options.selected_teams.size()!=options.local_players ||
       std::any_of(options.selected_teams.begin(),options.selected_teams.end(),
           [](auto team){return team<0 || team>7;})))
    throw FormatError("Team selections must provide one source team 0..7 per local player");
  rules.weapon=cache.slayer_starting_weapon(rules.custom==CustomSlayer::none?rules.starting_weapon:5,rules.source_engine);
  if(rules.custom!=CustomSlayer::none) {
    rules.weapon_choices=cache.multiplayer_weapon_choices();
    if(rules.custom==CustomSlayer::infection && options.local_players<2 && !options.host_port && options.join.empty() && !options.online_match && !options.dedicated)throw FormatError("Infection requires at least two players");
    if(options.source_object_world)throw FormatError("Custom Slayer rulesets require the native authoritative match");
  }
  if(options.session_random_seed)rules.random_seed=*options.session_random_seed;
  return rules;
}
std::shared_ptr<const std::filesystem::path> source_storage(
    const Cache &cache,const MultiplayerOptions &options,const SlayerRules &rules,bool shadow=false) {
  if(shadow&&(!cache.original_data()||cache.original_data()->image!=OriginalData::Image::TitleUpdate15||rules.custom!=CustomSlayer::none))
    throw FormatError("Source shadow requires actual Xbox TU1.5 gameplay and a supported source engine");
  // Clients consume the authority's WORLD nodes and completed death observer.
  if(!shadow&&(!options.join.empty() || (options.online_match &&
      options.online_match->host()!=options.online_match->self())))return {};
  const bool automatic=cache.original_data() &&
      cache.original_data()->image==OriginalData::Image::TitleUpdate15 && rules.custom==CustomSlayer::none &&
      !options.replay && !options.capture_root && options.reference_player.empty();
  if(!shadow&&!automatic && !options.source_object_world)return {};
  if(cache.is_mcc())throw FormatError("MCC native world startup and complete Xbox TU1.5 gameplay conversion are incomplete; Xbox reference executables are not runtime inputs");
  if(options.gameplay_xbe.empty())throw FormatError("Source world requires its authenticated executable");
  if(!shadow&&!options.source_object_savegame_directory.empty()) {
    std::filesystem::create_directories(options.source_object_savegame_directory);
    return std::make_shared<const std::filesystem::path>(options.source_object_savegame_directory);
  }
#if defined(__linux__)
  auto name=(std::filesystem::temp_directory_path()/"halo2-world-XXXXXX").string();
  const auto directory=::mkdtemp(name.data());
  if(!directory)throw FormatError("Could not create source world scratch storage");
  return {new std::filesystem::path(directory),[](const std::filesystem::path *path) {
    std::error_code ignored;std::filesystem::remove_all(*path,ignored);delete path;
  }};
#else
  throw FormatError("Native source world platform is unavailable");
#endif
}
std::unique_ptr<MultiplayerObjectRuntime> object_world(
    const Cache &cache,const MultiplayerOptions &options,const SlayerRules &rules,
    const std::shared_ptr<const std::filesystem::path> &storage,
    std::shared_ptr<PhysicsLiveClockSource> clock={}) {
  if(!storage)return {};
  auto world=clock?std::make_unique<MultiplayerObjectRuntime>(options.gameplay_xbe,cache,options.bsp,*storage,std::move(clock)):
      std::make_unique<MultiplayerObjectRuntime>(options.gameplay_xbe,cache,options.bsp,*storage);
  auto variant=options.variant;
  if(options.starting_weapon)variant.values.set(0x59,int(options.starting_weapon));
  world->configure_session(source_game_variant(variant),1,30,rules.random_seed);
  if(options.reference_auxiliary_seed) {
    if(options.input_script.empty() || !options.capture_tick || options.frames || options.ticks ||
        options.screenshot.empty() || options.host_port || !options.join.empty() ||
        options.dedicated || options.replay || options.online_match)
      throw FormatError("Reference source RNG input requires an isolated finite local script capture");
    // Declared finite replay input for the constructed146240 auxiliary word,
    // delivered before scenario factories consume the original source stream.
    auto &memory=world->memory();const auto rng=Bytes(memory.view(0x4f3cd8u,4)).u32(0);
    if(!rng)throw FormatError("Reference source RNG requires its constructed owner");
    memory.view(rng,8);const auto seed=*options.reference_auxiliary_seed;
    memory.write(rng+4u,std::as_bytes(std::span(&seed,1)));
  }
  return world;
}
}
struct MultiplayerPreparedWorld::SourceShadowSetup {
  std::vector<OnlineAdmission> roster;
  std::vector<unsigned> locals,selected;
  std::shared_ptr<PhysicsLiveClockSource> clock;
};
std::unique_ptr<MultiplayerPreparedWorld> MultiplayerPreparedWorld::source_shadow(
    const MultiplayerOptions &options,SourceShadowViews views,std::shared_ptr<PhysicsLiveClockSource> clock) {
  const auto &session=options.online_match;
  const auto phase=session?session->phase():OnlineMatchSession::Phase::Failed;
  const auto context=session?session->authority_context():std::nullopt;
  if(!session||!session->direct_udp()||!context||context->host>=16||options.host_port||!options.join.empty()||
      phase==OnlineMatchSession::Phase::Connecting||phase==OnlineMatchSession::Phase::Reviewing||phase==OnlineMatchSession::Phase::Postgame||
      options.defer_initial_player_spawn||options.player_birth_owner||options.pre_player_update_owner||
      !options.native_spawn_inputs.empty()||options.replay||!options.reference_player.empty()||options.reference_auxiliary_seed)
    throw FormatError("Source shadow requires an accepted direct ONLINE source session");
  if(views!=SourceShadowViews::LocalPeer&&views!=SourceShadowViews::IncomingHost)
    throw FormatError("Unknown source shadow view owner");
  SourceShadowSetup setup;setup.roster=session->roster();setup.selected=session->local_slots();setup.clock=std::move(clock);
  const auto peer=views==SourceShadowViews::LocalPeer?session->self():context->host;
  for(const auto &player:setup.roster)if(online_admission_peer(player)==peer)setup.locals.push_back(player.slot);
  if(setup.locals.empty()||setup.locals.size()>4||setup.selected.size()!=options.local_players)
    throw FormatError("Source shadow views differ from the accepted couch roster");
  return std::unique_ptr<MultiplayerPreparedWorld>(new MultiplayerPreparedWorld(options,std::nullopt,&setup));
}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options)
  :MultiplayerPreparedWorld(options,std::nullopt) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input)
  :MultiplayerPreparedWorld(options,std::move(script_input),nullptr) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input,const SourceShadowSetup *shadow)
  :cache(std::make_shared<const Cache>(options.map,options.shared_maps,
       options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:
           OriginalData::load_xbe(options.gameplay_xbe))),
   tags(world_tags(cache,options.bsp)),
   content(world_content(*cache,options,script_input)),rules(world_rules(*cache,options)),
   source_savegames(source_storage(*cache,options,rules,shadow!=nullptr)),
   source_objects(object_world(*cache,options,rules,source_savegames,shadow?shadow->clock:nullptr)),
   match(content,options.bsp,options.native_spawn_inputs.empty()?cache->spawns():
       std::span<const Spawn>(options.native_spawn_inputs),rules,{}, {},source_objects.get()),map_(options.map),shared_(options.shared_maps),
   xbe_(options.gameplay_xbe),bsp_(options.bsp),players_(options.local_players),
   starting_weapon_(options.starting_weapon),variant_(source_game_variant(options.variant)),
   teams_(options.selected_teams),profiles_(options.selected_profiles),seed_(options.session_random_seed),
   source_object_world_(options.source_object_world),source_object_savegame_directory_(options.source_object_savegame_directory),
   native_spawn_inputs_(options.native_spawn_inputs),shadow_(shadow!=nullptr) {
  if(!options.native_spawn_inputs.empty() && (options.online_match || options.host_port || !options.join.empty() || options.dedicated))
    throw FormatError("Declared native spawn inputs require an offline acceptance world");
  if(options.local_players>4 || (!options.dedicated && !options.local_players))
    throw FormatError("Multiplayer needs 1..4 local players");
  if(options.defer_initial_player_spawn || options.player_birth_owner || options.pre_player_update_owner) {
    if(options.online_match || options.host_port || !options.join.empty() || options.dedicated)
      throw FormatError("Source player activation policy requires an offline local roster");
    match.set_source_player_activation_owner(options.defer_initial_player_spawn,options.player_birth_owner,options.pre_player_update_owner);
  }
  match.configure_weapon_pickups();
  if(cache->original_data() && cache->original_data()->melee_constants)
    match.configure_melee_animation(cache,*cache->original_data()->melee_constants);
  if(cache->original_data() && cache->original_data()->grenade_constants)
    match.configure_grenades(cache,*cache->original_data()->grenade_constants);
  std::vector<FirstPersonBindingDefinition> definitions;
  if(options.host_port || options.online_match)
    for(unsigned representation:{2u,3u})
      if(representation<content.player_representations.size() &&
          content.player_representations[representation].biped.datum!=null_datum)
        for(const auto &[datum,weapon]:content.weapons)if(!weapon.first_person_models.empty() && weapon.first_person_models.front().model!=null_datum && weapon.first_person_models.front().animation!=null_datum)
          {auto definition=cache->load_first_person_binding(datum,representation);
            match.configure_replicated_first_person(definition);if(shadow)definitions.push_back(std::move(definition));}
  if(shadow) {
    for(const auto &player:shadow->roster){const auto selected=std::ranges::find(shadow->selected,player.slot);
      std::optional<SourcePlayerConfiguration> profile;
      if(selected!=shadow->selected.end()&&!options.selected_profiles.empty())profile=options.selected_profiles.at(selected-shadow->selected.begin());
      if(match.join(player.name,rules.team_mode?player.team:-1,profile,player.slot)!=player.slot)
        throw FormatError("Source shadow birth changed an accepted native player slot");
    }
    local_slots=shadow->locals;match.configure_local_views(local_slots,definitions);
  }else if(!options.online_match && options.join.empty())
    for(unsigned i=0;i<options.local_players;++i)
      local_slots.push_back(match.join(options.selected_profiles.empty()?"Player "+std::to_string(i+1):
          native_player_configuration_name(options.selected_profiles[i]),
          options.selected_teams.empty()?-1:options.selected_teams[i],
          options.selected_profiles.empty()?std::nullopt:std::optional(options.selected_profiles[i])));
}
bool MultiplayerPreparedWorld::matches(const MultiplayerOptions& options) const {
  const bool same_spawns=native_spawn_inputs_.size()==options.native_spawn_inputs.size() &&
      std::equal(native_spawn_inputs_.begin(),native_spawn_inputs_.end(),options.native_spawn_inputs.begin(),[](const auto&a,const auto&b){
        return a.position.x==b.position.x && a.position.y==b.position.y && a.position.z==b.position.z &&
            a.facing==b.facing && a.bsp==b.bsp && a.team==b.team && a.player_type==b.player_type &&
            a.game_types==b.game_types && a.spawn_types==b.spawn_types;
      });
  // Constructor-only birth/defer/pre-step policy already belongs to this Match.
  // Runtime reuses that owner and cannot replace or replay the callbacks.
  return !shadow_&&map_==options.map && shared_==options.shared_maps && xbe_==options.gameplay_xbe &&
      bsp_==options.bsp && players_==options.local_players && starting_weapon_==options.starting_weapon &&
      variant_.bytes==source_game_variant(options.variant).bytes && teams_==options.selected_teams &&
      profiles_==options.selected_profiles && seed_==options.session_random_seed &&
      source_object_world_==options.source_object_world &&
      source_object_savegame_directory_==options.source_object_savegame_directory && same_spawns;
}
} // namespace halo2
