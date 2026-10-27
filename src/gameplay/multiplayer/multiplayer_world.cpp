#include "halo2/multiplayer_world.hpp"
#include "halo2/native_player_profile.hpp"
#include "halo2/online_match_session.hpp"
#include "halo2/gameplay_font_source.hpp"
#include "halo2/source_runtime_resource_preparation.hpp"
#include "halo2/hud.hpp"
#include "halo2/native_weapon_hud.hpp"
#include <algorithm>
#include <cfenv>
#include <condition_variable>
#include <cstdlib>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <atomic>
#include <random>
#endif

namespace halo2 {
namespace {
TagSystem world_tags(std::shared_ptr<const Cache> cache, std::size_t bsp) {
  TagSystem tags(std::move(cache));
  tags.load_bsp(bsp);
  return tags;
}
LevelContent world_content(const Cache &cache,const MultiplayerOptions &options,
    std::optional<ScriptProgram> &script_input) {
  if(options.campaign_difficulty && (*options.campaign_difficulty<0 || *options.campaign_difficulty>3))
    throw FormatError("Campaign physical roster requires original difficulty0..3");
  auto content=cache.load_content(options.bsp,!options.campaign_difficulty,source_game_variant(options.variant));
  if(script_input) {
    if(options.online_match || options.host_port || !options.join.empty() || options.dedicated)
      throw FormatError("Declared script inputs require an offline acceptance world");
    content.scripts=std::move(*script_input);
  }
  return content;
}
SlayerRules world_rules(const Cache& cache,const MultiplayerOptions& options,const LevelContent &content) {
  auto variant=options.variant;
  if(options.starting_weapon)variant.values.set(0x59,int(options.starting_weapon));
  const bool source_variant=cache.original_data() && cache.original_data()->image==OriginalData::Image::TitleUpdate15 &&
      !options.campaign_difficulty && custom_slayer(variant)==CustomSlayer::none && !bomb_defusal_variant(variant) &&
      options.native_spawn_inputs.empty() && !options.replay && !options.capture_root && options.reference_player.empty();
  auto rules=source_variant?multiplayer_source_rules(variant,true):slayer_rules(variant,true);
  if(!options.selected_profiles.empty() && options.selected_profiles.size()!=options.local_players)
    throw FormatError("Selected profiles must provide one complete record per local player");
  if((!options.selected_teams.empty() || rules.team_mode) &&
      (options.selected_teams.size()!=options.local_players ||
       std::any_of(options.selected_teams.begin(),options.selected_teams.end(),
           [](auto team){return team<0 || team>7;})))
    throw FormatError("Team selections must provide one source team 0..7 per local player");
  rules.weapon=options.campaign_difficulty ? content.starting_weapon : rules.requires_source_engine ? null_datum :
       cache.slayer_starting_weapon(rules.custom==CustomSlayer::none?rules.starting_weapon:5,rules.source_engine);
  const bool source_campaign=options.campaign_difficulty && cache.original_data() &&
      cache.original_data()->image==OriginalData::Image::TitleUpdate15;
  if(rules.weapon==null_datum && !source_campaign && !rules.requires_source_engine)
    throw FormatError("Selected session has no authored starting weapon");
  if(rules.custom!=CustomSlayer::none || rules.defusal) {
    rules.weapon_choices=cache.multiplayer_weapon_choices();
    if(rules.custom==CustomSlayer::infection && options.local_players<2 && !options.host_port && options.join.empty() && !options.online_match && !options.dedicated)throw FormatError("Infection requires at least two players");
    if(options.source_object_world)throw FormatError("Custom Slayer rulesets require the native authoritative match");
  }
  if(options.session_random_seed)rules.random_seed=*options.session_random_seed;
  if(rules.defusal && std::ranges::none_of(variant.values,[](const auto &value){
      return value.first>=native_defusal_site_parameter && value.first<native_defusal_site_parameter+16;
    }))rules.defusal_layout=bundled_bomb_defusal_layout(cache.scenario(),content.netgame_flags);
  return rules;
}
std::shared_ptr<const std::filesystem::path> source_storage(
    const Cache &cache,const MultiplayerOptions &options,const SlayerRules &rules,bool shadow=false) {
  if(shadow&&(!cache.original_data()||cache.original_data()->image!=OriginalData::Image::TitleUpdate15||rules.custom!=CustomSlayer::none||rules.defusal))
    throw FormatError("Source shadow requires actual Xbox TU1.5 gameplay and a supported source engine");
  // Clients consume the authority's WORLD nodes and completed death observer.
  if(!shadow&&(!options.join.empty() || (options.online_match &&
      options.online_match->host()!=options.online_match->self())))return {};
  const bool automatic=cache.original_data() &&
      cache.original_data()->image==OriginalData::Image::TitleUpdate15 && rules.custom==CustomSlayer::none && !rules.defusal &&
      !options.replay && !options.capture_root && options.reference_player.empty();
  if(!shadow&&!automatic && !options.source_object_world)return {};
  if(options.gameplay_xbe.empty() && !cache.is_mcc())
    throw FormatError("Source reference world requires its authenticated executable");
  if(!shadow&&!options.source_object_savegame_directory.empty()) {
    std::filesystem::create_directories(options.source_object_savegame_directory);
    return std::make_shared<const std::filesystem::path>(options.source_object_savegame_directory);
  }
#if defined(_WIN32)
  const auto parent=std::filesystem::temp_directory_path();
  std::random_device random;static std::atomic<unsigned> next{};
  for(unsigned attempt=0;attempt<32;++attempt) {
    const auto directory=parent/("halo2-world-"+std::to_string(random())+"-"+std::to_string(next.fetch_add(1)));
    std::error_code error;
    if(std::filesystem::create_directory(directory,error))
      return {new std::filesystem::path(directory),[](const std::filesystem::path* path) {
        std::error_code ignored;std::filesystem::remove_all(*path,ignored);delete path;
      }};
    if(error && error!=std::errc::file_exists)break;
  }
  throw FormatError("Could not create source world scratch storage");
#elif defined(__linux__)
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
    std::shared_ptr<PhysicsLiveClockSource> clock={},std::shared_ptr<SourceExecutionInputs> inputs={}) {
  if(inputs && (!storage || options.online_match || options.host_port || !options.join.empty() ||
      options.dedicated || options.replay))
    throw FormatError("Constructor operand tape requires an isolated local native world");
  if(!storage)return {};
  auto world=inputs?std::make_unique<MultiplayerObjectRuntime>(options.gameplay_xbe,cache,options.bsp,*storage,std::move(clock),std::move(inputs)):
      options.gameplay_xbe.empty()?
      std::make_unique<MultiplayerObjectRuntime>(cache,options.bsp,*storage,std::move(clock)):
      std::make_unique<MultiplayerObjectRuntime>(options.gameplay_xbe,cache,options.bsp,*storage,std::move(clock));
  auto variant=options.variant;
  if(options.starting_weapon)variant.values.set(0x59,int(options.starting_weapon));
  if(options.campaign_difficulty) {
    world->tu15_world().initialize_map();
    world->tu15_world().configure_campaign_session(*options.campaign_difficulty,1,30,rules.random_seed);
  } else world->configure_session(source_game_variant(variant),1,30,rules.random_seed);
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
  if(options.source_world_observer)options.source_world_observer(world->memory());
  return world;
}
bool source_replica_world(const Cache &cache,const MultiplayerOptions &options,
    const MultiplayerObjectRuntime *world) {
  return !world && cache.original_data() && cache.original_data()->image==OriginalData::Image::TitleUpdate15 &&
      (!options.join.empty() || (options.online_match && options.online_match->host()!=options.online_match->self()));
}
std::function<std::uint32_t()> result_clock_source(const std::shared_ptr<PhysicsLiveClockSource> &clock) {
  if(!clock)return {};
  // The native provider's FILETIME domain is100ns since1601, as in
  // NativeWorldClock. Keep Match's separate32-bit Unix/PCR domain while
  // allowing an explicitly supplied source to own both physical inputs.
  return [clock]{return std::uint32_t(std::int64_t(clock->utc_filetime()/10'000'000ull)-11'644'473'600ll);};
}
}
struct MultiplayerMapAssets::Huds {
  std::map<std::pair<std::uint32_t,std::uint32_t>,HudContent> content;
};
MultiplayerMapAssets::MultiplayerMapAssets(const MultiplayerOptions& options)
    :map_(options.map),shared_(options.shared_maps),xbe_(options.gameplay_xbe),
     bsp_(options.bsp),variant_(source_game_variant(options.variant)),custom_fingerprint_(custom_slayer_fingerprint(0,options.variant)),campaign_difficulty_(options.campaign_difficulty) {
  if(options.online_match || options.host_port || !options.join.empty() || options.dedicated ||
     options.replay || !options.input_script.empty() || options.prepared_world)
    throw FormatError("Countdown assets require a fresh offline map request");
  cache_=std::make_shared<const Cache>(options.map,options.shared_maps,
      options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:OriginalData::load_xbe(options.gameplay_xbe));
  std::optional<ScriptProgram> script;
  content_.emplace(world_content(*cache_,options,script));
  mesh_.emplace(cache_->load_bsp(options.bsp));
  resources_=std::make_shared<SourceRuntimeResourcePreparation>(cache_);
  fonts=std::make_shared<GameplayFontBank>(gameplay_font_directory(*cache_));
  for(const auto selector:{6u,8u,5u})(void)fonts->font(selector);
  if(cache_->is_mcc())for(const auto selector:{6u,5u})(void)fonts->raw(selector);
  huds_=std::make_unique<Huds>();
  if(!options.campaign_difficulty)
    for(const auto representation:source_session_player_representations(*content_,2)) {
      const auto biped=multiplayer_biped(*content_,representation).datum;
      const auto add=[&](std::uint32_t weapon) {
        const auto key=std::pair{biped,weapon};
        if(!huds_->content.contains(key))huds_->content.emplace(key,cache_->load_hud(biped,weapon));
      };
      add(null_datum);
      for(const auto& binding:handheld_weapon_hud_bindings(*cache_,representation))add(binding.definition);
    }
}
MultiplayerMapAssets::~MultiplayerMapAssets()=default;
HudContent MultiplayerMapAssets::take_hud(std::uint32_t biped,std::uint32_t weapon) {
  const auto at=huds_->content.find({biped,weapon});
  if(at==huds_->content.end())return cache_->load_hud(biped,weapon);
  auto content=std::move(at->second);huds_->content.erase(at);return content;
}
float MultiplayerMapAssets::hud_sensor_range(std::uint32_t biped,std::uint32_t weapon) const {
  const auto at=huds_->content.find({biped,weapon});
  return at==huds_->content.end()?cache_->load_hud(biped,weapon).sensor.range:at->second.sensor.range;
}
bool MultiplayerMapAssets::matches(const MultiplayerOptions& options) const {
  return map_==options.map && shared_==options.shared_maps && xbe_==options.gameplay_xbe && bsp_==options.bsp &&
      variant_.bytes==source_game_variant(options.variant).bytes && custom_fingerprint_==custom_slayer_fingerprint(0,options.variant) && campaign_difficulty_==options.campaign_difficulty &&
      !options.online_match && !options.host_port && options.join.empty() && !options.dedicated && !options.replay;
}
std::shared_ptr<const Cache> MultiplayerMapAssets::cache_for(const MultiplayerOptions& options) const {
  if(!matches(options))throw FormatError("Prepared map resources belong to a different request");
  return cache_;
}
LevelContent MultiplayerMapAssets::take_content(const MultiplayerOptions& options,std::optional<ScriptProgram>& script) {
  if(!matches(options) || !content_)throw FormatError("Prepared map resources are mismatched or already consumed");
  auto content=std::move(*content_);content_.reset();
  if(script)content.scripts=std::move(*script);
  return content;
}
Mesh MultiplayerMapAssets::take_bsp_mesh(const MultiplayerOptions& options) {
  if(!matches(options))throw FormatError("Prepared BSP mesh belongs to a different request");
  if(!mesh_)return cache_->load_bsp(options.bsp);
  auto mesh=std::move(*mesh_);mesh_.reset();return mesh;
}
struct MultiplayerAssetLoader::Impl {
  struct Job {
    MultiplayerOptions options;
    std::fenv_t environment;
    std::promise<std::shared_ptr<MultiplayerMapAssets>> promise;
  };
  std::mutex mutex;
  std::condition_variable available;
  bool stopping{};
  std::optional<Job> pending;
  std::thread worker;
  Impl():worker([this]{run();}) {}
  ~Impl() {
    {std::lock_guard lock(mutex);stopping=true;pending.reset();}
    available.notify_one();worker.join();
  }
  void run() {
    for(;;) {
      std::optional<Job> job;
      {
        std::unique_lock lock(mutex);
        available.wait(lock,[&]{return stopping || pending.has_value();});
        if(stopping)return;
        job=std::move(pending);pending.reset();
      }
      try {
        if(std::fesetenv(&job->environment))throw FormatError("Cannot retain map preparation floating-point context");
        job->promise.set_value(std::make_shared<MultiplayerMapAssets>(job->options));
      }catch(...) {job->promise.set_exception(std::current_exception());}
    }
  }
};
MultiplayerAssetLoader::MultiplayerAssetLoader():impl_(std::make_unique<Impl>()) {}
MultiplayerAssetLoader::~MultiplayerAssetLoader()=default;
std::shared_future<std::shared_ptr<MultiplayerMapAssets>> MultiplayerAssetLoader::request(const MultiplayerOptions& options) {
  if(options.online_match || options.host_port || !options.join.empty() || options.dedicated ||
     options.replay || !options.input_script.empty() || options.prepared_world)
    throw FormatError("Countdown assets require a fresh offline map request");
  // Copy resource selectors only. The worker never retains or invokes player,
  // simulation, platform-device, presentation or source-birth callbacks.
  Impl::Job job;
  job.options.map=options.map;job.options.shared_maps=options.shared_maps;job.options.gameplay_xbe=options.gameplay_xbe;
  job.options.bsp=options.bsp;job.options.variant=options.variant;job.options.campaign_difficulty=options.campaign_difficulty;
  if(std::fegetenv(&job.environment))throw FormatError("Cannot capture map preparation floating-point context");
  auto result=job.promise.get_future().share();
  {std::lock_guard lock(impl_->mutex);impl_->pending=std::move(job);}
  impl_->available.notify_one();return result;
}
struct MultiplayerPreparedWorld::SourceShadowSetup {
  std::vector<OnlineAdmission> roster;
  std::vector<unsigned> locals,selected;
  std::shared_ptr<PhysicsLiveClockSource> clock;
};
std::unique_ptr<MultiplayerPreparedWorld> MultiplayerPreparedWorld::source_shadow(
    const MultiplayerOptions &options,SourceShadowViews views,std::shared_ptr<PhysicsLiveClockSource> clock) {
  return source_shadow(options,views,std::move(clock),{});
}
std::shared_ptr<const Cache> MultiplayerPreparedWorld::original_cache() const {
  if(cache_executor_!=std::this_thread::get_id() || !constructor_cache_ ||
      cache.get()!=constructor_cache_.get() || cache.owner_before(constructor_cache_) ||
      constructor_cache_.owner_before(cache))
    throw FormatError("Incoming native world requires its creator and original Cache owner");
  return constructor_cache_;
}
std::unique_ptr<MultiplayerPreparedWorld> MultiplayerPreparedWorld::incoming_source_shadow(
    const MultiplayerOptions &options,const MultiplayerPreparedWorld &resident,
    std::shared_ptr<PhysicsLiveClockSource> clock) {
  auto cache=resident.original_cache();
  return source_shadow(options,SourceShadowViews::IncomingHost,std::move(clock),std::move(cache));
}
std::unique_ptr<MultiplayerPreparedWorld> MultiplayerPreparedWorld::source_shadow(
    const MultiplayerOptions &options,SourceShadowViews views,std::shared_ptr<PhysicsLiveClockSource> clock,
    std::shared_ptr<const Cache> incoming_cache) {
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
  return std::unique_ptr<MultiplayerPreparedWorld>(new MultiplayerPreparedWorld(options,std::nullopt,&setup,setup.clock,{},std::move(incoming_cache)));
}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options)
  :MultiplayerPreparedWorld(options,std::nullopt) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::shared_ptr<MultiplayerMapAssets> assets)
  :MultiplayerPreparedWorld(options,std::nullopt,nullptr,{},std::move(assets)) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input)
  :MultiplayerPreparedWorld(options,std::move(script_input),nullptr,{}) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input,std::shared_ptr<PhysicsLiveClockSource> clock)
  :MultiplayerPreparedWorld(options,std::move(script_input),nullptr,std::move(clock)) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input,std::shared_ptr<PhysicsLiveClockSource> clock,
    std::shared_ptr<SourceExecutionInputs> inputs)
  :MultiplayerPreparedWorld(options,std::move(script_input),nullptr,std::move(clock),{},{},std::move(inputs)) {}
MultiplayerPreparedWorld::MultiplayerPreparedWorld(const MultiplayerOptions& options,
    std::optional<ScriptProgram> script_input,const SourceShadowSetup *shadow,std::shared_ptr<PhysicsLiveClockSource> clock,
    std::shared_ptr<MultiplayerMapAssets> assets,std::shared_ptr<const Cache> incoming_cache,
    std::shared_ptr<SourceExecutionInputs> inputs)
  :constructor_cache_(incoming_cache?std::move(incoming_cache):assets?assets->cache_for(options):std::make_shared<const Cache>(options.map,options.shared_maps,
       options.gameplay_xbe.empty()?std::shared_ptr<const OriginalData>{}:
           OriginalData::load_xbe(options.gameplay_xbe))),
   cache(constructor_cache_),
   tags(world_tags(cache,options.bsp)),
   content(assets?assets->take_content(options,script_input):world_content(*cache,options,script_input)),rules(world_rules(*cache,options,content)),
   source_savegames(source_storage(*cache,options,rules,shadow!=nullptr)),
   source_objects(object_world(*cache,options,rules,source_savegames,clock?clock:(shadow?shadow->clock:nullptr),std::move(inputs))),
   match(content,options.bsp,options.native_spawn_inputs.empty()?cache->spawns():
       std::span<const Spawn>(options.native_spawn_inputs),rules,{},result_clock_source(clock),source_objects.get(),options.campaign_difficulty ?
        std::optional(SourceSceneGameContext{1,rules.source_engine}):std::nullopt,options.campaign_spawn_input,source_replica_world(*cache,options,source_objects.get())),map_(options.map),shared_(options.shared_maps),
   xbe_(options.gameplay_xbe),bsp_(options.bsp),players_(options.local_players),
   starting_weapon_(options.starting_weapon),variant_(source_game_variant(options.variant)),custom_fingerprint_(custom_slayer_fingerprint(0,options.variant)),
   teams_(options.selected_teams),profiles_(options.selected_profiles),seed_(options.session_random_seed),
   source_object_world_(options.source_object_world),campaign_difficulty_(options.campaign_difficulty),source_object_savegame_directory_(options.source_object_savegame_directory),
   native_spawn_inputs_(options.native_spawn_inputs),shadow_(shadow!=nullptr) {
  match.bind_native_first_person_cache(cache);
  if(assets){gameplay_fonts=assets->fonts;map_assets=std::move(assets);}
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
  // Host netcode mode (H2_NET_MODE overrides). Original installs nothing;
  // a world without remote views keeps no lag-compensation history. Source
  // shadows are configured like the host: they replay its rewound steps and
  // can be promoted, and an imported cut adopts the producer's mode and
  // rewind history. Joining clients only retain the mode value.
  match.configure_netcode(resolve_netcode_mode(options.netcode_mode),cache,options.bsp,
      shadow || (options.join.empty() &&
      (!options.online_match || options.online_match->host()==options.online_match->self())));
  if(cache->original_data() && cache->original_data()->melee_constants)
    match.configure_melee_animation(cache,*cache->original_data()->melee_constants);
  if(cache->original_data() && cache->original_data()->grenade_constants)
    match.configure_grenades(cache,*cache->original_data()->grenade_constants);
  if(options.host_port || options.online_match)
    for(unsigned representation:source_session_player_representations(content,match.source_game_type()))
      if(representation<content.player_representations.size() &&
          content.player_representations[representation].biped.datum!=null_datum)
        for(const auto &[datum,weapon]:content.weapons)if(!weapon.first_person_models.empty())
          {const auto binding=cache->load_first_person_binding(datum,representation);if(binding.record_present && binding.weapon_model!=null_datum && binding.graph!=null_datum)match.configure_replicated_first_person(binding);}
  if(shadow) {
    for(const auto &player:shadow->roster){const auto selected=std::ranges::find(shadow->selected,player.slot);
      std::optional<SourcePlayerConfiguration> profile;
      if(selected!=shadow->selected.end()&&!options.selected_profiles.empty())profile=options.selected_profiles.at(selected-shadow->selected.begin());
      if(match.join(player.name,rules.team_mode?player.team:-1,profile,player.slot)!=player.slot)
        throw FormatError("Source shadow birth changed an accepted native player slot");
    }
    bind_source_local_views(shadow->locals);
    if(match.source_engine_owned_ && !options.defer_initial_player_spawn)match.step({});
  }else if(!options.online_match && options.join.empty())
    for(unsigned i=0;i<options.local_players;++i)
      local_slots.push_back(match.join(options.selected_profiles.empty()?"Player "+std::to_string(i+1):
          native_player_configuration_name(options.selected_profiles[i]),
          options.selected_teams.empty()?-1:options.selected_teams[i],
          options.selected_profiles.empty()?std::nullopt:std::optional(options.selected_profiles[i])));
  if(!shadow && !local_slots.empty() && match.source_engine_owned_) {
    //14F620's genuine birth continuation requires the selected local/input
    // user before it constructs18A0C0 and16B16A. Deferred views already admit
    // current_unit=NONE; keep this identity in place for the first engine tick.
    const auto locals=local_slots;bind_source_local_views(locals);
    if(!options.defer_initial_player_spawn)match.step({});
  }
  if(match.source_engine_owned_) {
    match.first_person_setup_tick_=match.simulation_.ticks();
    // Existing presentation templates need a weapon reference. Observe one
    // genuinely born unit after startup; this never chooses a loadout or
    // substitutes Map Default for an original random/absent selection.
    // Keep this observation in the presentation rules. Match retains its
    // admitted rules so its configuration identity does not depend on birth
    // timing or whether this constructor is preparing a source shadow.
    for(unsigned slot=0;slot<multiplayer_capacity;++slot)
      if(match.players_[slot].connected && match.players_[slot].source_current_object!=null_datum) {
        rules.weapon=match.unit(slot).weapon.definition;break;
      }
  }
}
std::uint32_t MultiplayerPreparedWorld::network_starting_weapon_identity() const {
  if(!cache->is_mcc())return rules.weapon;
  const auto &configuration=match.session_configuration();
  if(!configuration)throw FormatError("MCC loading identity requires its admitted configuration");
  return std::uint32_t(source_game_variant_get(source_game_variant(*configuration),89));
}
void MultiplayerPreparedWorld::bind_source_local_views(std::span<const unsigned> locals) {
  if(!source_objects||!source_objects->owns_physics()||!cache->original_data()||cache->original_data()->image!=OriginalData::Image::TitleUpdate15)
    throw FormatError("Source local views require this world's actual TU1.5 owner");
  std::vector<FirstPersonBindingDefinition> definitions;
  for(const auto &[datum,weapon]:content.weapons)
    if(!weapon.first_person_models.empty())
      for(unsigned representation:source_session_player_representations(content,match.source_game_type()))if(representation<content.player_representations.size()&&content.player_representations[representation].biped.datum!=null_datum)
        {const auto binding=cache->load_first_person_binding(datum,representation);if(binding.record_present && binding.weapon_model!=null_datum && binding.graph!=null_datum)definitions.push_back(binding);}
  match.configure_local_views(locals,definitions);local_slots.assign(locals.begin(),locals.end());
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
      variant_.bytes==source_game_variant(options.variant).bytes && custom_fingerprint_==custom_slayer_fingerprint(0,options.variant) && teams_==options.selected_teams &&
      profiles_==options.selected_profiles && seed_==options.session_random_seed &&
      source_object_world_==options.source_object_world &&
      campaign_difficulty_==options.campaign_difficulty &&
      source_object_savegame_directory_==options.source_object_savegame_directory && same_spawns;
}
} // namespace halo2
