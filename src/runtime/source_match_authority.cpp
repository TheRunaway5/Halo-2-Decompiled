#include "halo2/source_match_authority.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_player_departure.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_game_variant.hpp"
#include <algorithm>
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &bank,std::uint32_t at) {
  const auto bytes=bank.view(at,4);std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return value;
}
template<class State>void validate_pair(const State &match,const SourceRuntimeMemory &bank,
    const MultiplayerMatch &owner,std::span<const unsigned> local_players) {
  // Same authenticated TU1.5 globals used by PhysicsLiveWorldTu15::source_clock
  // and source_player_pool. These are original data addresses, never imported
  // NativeRuntimeAddress values (which refer to this process's allocations).
  const auto clock=word(bank,0x51df54u),pool=word(bank,0x4f54f4u),census=word(bank,0x4f54f0u);
  if(!match.source_backed||!match.source_players||!clock||clock>UINT32_MAX-12u||!pool||
      match.source_players->pool!=pool||word(bank,clock+8u)!=std::uint32_t(match.simulation.ticks))
    throw FormatError("TU1.5 source world and native Match have different completed ticks or player pools");
  // configure_session delivers this exact native mode/rate/seed/variant to
  // 4F3218's game-options owner. Bind the cut to the locally admitted settings,
  // rather than accepting a different source engine behind the same map.
  const auto game=word(bank,0x4f3218u);
  if(!game||game>UINT32_MAX-0x26cu||word(bank,game+8u)!=std::uint32_t(owner.source_game_type())||word(bank,game+0xcu)!=(1u|(30u<<16))||
      word(bank,game+0x18u)!=owner.presentation_seed()||!owner.session_configuration())
    throw FormatError("TU1.5 source world has different admitted session/clock inputs");
  const auto variant=source_game_variant(*owner.session_configuration());
  if(source_game_variant_type(variant)!=owner.source_engine()||
      !std::ranges::equal(bank.view(game+0x13cu,variant.bytes.size()),std::as_bytes(std::span(variant.bytes))))
    throw FormatError("TU1.5 source world has a different admitted game variant");
  const SourceRawDatumPoolServices storage{.read=[&](auto address,auto bytes){return bank.view(address,bytes);}};
  unsigned active_players=0;
  for(const auto &player:match.players)if(player.source_datum!=null_datum) {
    const auto record=source_raw_datum_lookup(pool,player.source_datum,storage);
    if(!record)throw FormatError("TU1.5 coupled Match lost its source player allocation");
    SourceDeparturePlayer raw;const auto bytes=bank.view(record,sizeof(raw));std::memcpy(&raw,bytes.data(),sizeof(raw));
    if(raw.current_unit!=player.source_current_object)
      throw FormatError("TU1.5 source player and native Match have different current units");
    if(bool(raw.flags&2u)!=bool(player.source_flags&2u)||raw.configuration[0x7c]!=std::uint8_t(player.source_membership))
      throw FormatError("TU1.5 source player and native Match have different departure/team membership");
    active_players+=!(raw.flags&2u);
    // Native no-profile admission supplies the original default profile. Only
    // compare the complete profile when the Match retains a selected one.
    if(player.source_configuration&&raw.configuration!=*player.source_configuration)
      throw FormatError("TU1.5 source player and native Match have different selected configurations");
    const auto local=std::ranges::find(local_players,unsigned(player.source_datum&65535u));
    const auto expected=local!=local_players.end()&&!(raw.flags&2u)?std::int16_t(local-local_players.begin()):std::int16_t(-1);
    if(raw.local_index!=expected)
      throw FormatError("TU1.5 source player and native Match have different local view owners");
  }
  // The original birth/leave owners maintain an active census separate from
  // allocated retained players. A copied pool alone cannot repair this owner.
  if(!census||word(bank,census)!=active_players)
    throw FormatError("TU1.5 source active-player census differs from retained Match membership");
  for(unsigned view=0;view<4;++view) {
    auto expected=null_datum,unit=null_datum;
    if(view<local_players.size()) {
      const auto &player=match.players.at(local_players[view]);
      if(player.connected&&!(player.source_flags&2u)){expected=player.source_datum;unit=player.source_current_object;}
    }
    if(word(bank,census+0xcu+view*4u)!=expected)
      throw FormatError("TU1.5 source local-view map differs from the configured native roster");
    const auto first_person=word(bank,0x4f6498u);
    if(!first_person||word(bank,first_person+view*0x20ccu+4u)!=unit||match.first_person_views[view].unit!=unit)
      throw FormatError("TU1.5 source first-person unit differs from its local player binding");
  }
}
void validate_source_players(const MultiplayerMatch::AuthorityCheckpoint &saved,const SourceRuntimeMemory &bank,
    std::span<const SourcePlayerAdmission> roster,bool complete) {
  if(roster.empty()||roster.size()>multiplayer_capacity)throw FormatError("Replacement source roster exceeds native capacity or is empty");
  const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return bank.view(at,n);}};
  const auto pool=word(bank,0x4f54f4u);std::uint16_t allocated{};
  for(const auto &entry:roster){
    if(entry.slot>=multiplayer_capacity||(allocated&(1u<<entry.slot))||saved.players[entry.slot].source_datum!=entry.player)
      throw FormatError("Replacement source player differs from its independently accepted slot");
    allocated|=std::uint16_t(1u<<entry.slot);const auto row=source_raw_datum_lookup(pool,entry.player,storage);
    if(!row||!std::ranges::equal(bank.view(row+offsetof(SourceDeparturePlayer,identity),sizeof(entry.identity)),std::as_bytes(std::span(entry.identity))))
      throw FormatError("Replacement has a different full source player identity");
  }
  if(complete)for(unsigned slot=0;slot<multiplayer_capacity;++slot)
    if(bool(allocated&(1u<<slot))!=(saved.players[slot].source_datum!=null_datum))
      throw FormatError("Replacement source cut omitted or introduced an admitted player allocation");
}
}
SourceMatchAuthority::SourceMatchAuthority(MultiplayerMatch &match,MultiplayerObjectRuntime &world):match_(match),world_(world) {
  if(match.source_object_runtime()!=&world||!world.owns_physics()||!match.original_data()||
      match.original_data()->image!=OriginalData::Image::TitleUpdate15)
    throw FormatError("Coupled source authority requires this Match's actual TU1.5 world");
}
SourceMatchAuthority::Checkpoint SourceMatchAuthority::checkpoint(AuthorityDigest binding)const {
  calls_.capture();NativeAuthorityBoundary::Call active(calls_);Checkpoint saved;
  saved.match=match_.authority_checkpoint(binding,true);saved.world=world_.authority_checkpoint();
  validate_pair(saved.match,world_.memory(),match_,match_.first_person_players_);return saved;
}
std::uint64_t SourceMatchAuthority::execution_tick()const {
  calls_.capture();match_.authority_calls_.capture();world_.require_authority_executor(true);
  return match_.simulation().ticks();
}
SourceMatchAuthority::ExecutionBoundary SourceMatchAuthority::execution_boundary()const {
  const auto tick=execution_tick();const auto counters=match_.execution_call_counters();
  const auto native=world_.execution_mutations();
  struct Tick {std::uint64_t ticks;};
  struct LiveState {
    bool source_backed;std::optional<DatumPool::SourceBinding> source_players;Tick simulation;
    const std::array<MultiplayerPlayer,multiplayer_capacity> &players;
    const std::array<FirstPersonViewBinding,4> &first_person_views;
  } state{bool(match_.player_pool_.source_memory()),match_.player_pool_.source_binding(),{tick},match_.players_,match_.first_person_views_};
  validate_pair(state,world_.memory(),match_,match_.first_person_players_);
  const auto allocated=std::count_if(match_.players_.begin(),match_.players_.end(),[](const auto &p){return p.source_datum!=null_datum;});
  if(match_.player_pool_.count()!=allocated)throw FormatError("Native source player census differs from coupled Match allocations");
  return {tick,{*counters[0],*counters[1],native[0],native[1]}};
}
std::vector<SourcePlayerAdmission> SourceMatchAuthority::player_admissions(std::span<const unsigned> slots)const {
  calls_.capture();match_.authority_calls_.capture();NativeAuthorityBoundary::Call active(calls_);
  if(slots.empty()||slots.size()>multiplayer_capacity)throw FormatError("Source admissions require the accepted player slots");
  const auto &bank=world_.memory();const auto pool=world_.source_player_pool();
  const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return bank.view(at,n);}};
  std::vector<SourcePlayerAdmission> result;result.reserve(slots.size());std::uint16_t seen{};
  for(auto slot:slots) {
    if(slot>=multiplayer_capacity||(seen&(1u<<slot)))throw FormatError("Source admission has a duplicate or invalid player slot");
    seen|=std::uint16_t(1u<<slot);const auto &player=match_.player(slot);
    if(!player.connected||player.source_datum==null_datum||(player.source_datum&65535u)!=slot)
      throw FormatError("Source admission has no active allocated native player");
    const auto row=source_raw_datum_lookup(pool,player.source_datum,storage);
    if(!row)throw FormatError("Source admission lost its full native player allocation");
    SourcePlayerAdmission entry{slot,player.source_datum};
    bank.read(row+offsetof(SourceDeparturePlayer,identity),std::as_writable_bytes(std::span(entry.identity)));
    result.push_back(entry);
  }
  return result;
}
std::vector<SourcePlayerAdmission> SourceMatchAuthority::allocated_player_admissions()const {
  calls_.capture();match_.authority_calls_.capture();world_.require_authority_executor(true);
  NativeAuthorityBoundary::Call active(calls_);
  const auto &bank=world_.memory();const auto pool=world_.source_player_pool();
  const SourceRawDatumPoolServices storage{.read=[&](auto at,auto n){return bank.view(at,n);}};
  std::vector<SourcePlayerAdmission> result;
  for(unsigned slot=0;slot<multiplayer_capacity;++slot) {
    const auto datum=match_.player(slot).source_datum;
    if(datum==null_datum)continue;
    if((datum&65535u)!=slot)throw FormatError("Allocated source admission has a different native slot");
    const auto row=source_raw_datum_lookup(pool,datum,storage);
    if(!row)throw FormatError("Allocated source admission lost its full native allocation");
    SourcePlayerAdmission entry{slot,datum};
    bank.read(row+offsetof(SourceDeparturePlayer,identity),std::as_writable_bytes(std::span(entry.identity)));
    result.push_back(entry);
  }
  return result;
}
struct SourceMatchAuthority::Prepared::Impl {
  SourceMatchAuthority *owner{};bool *staging{};
  std::optional<MultiplayerObjectRuntime::PreparedAuthority> world;
  std::optional<MultiplayerMatch::PreparedAuthority> match;
  // Destroy native borrowed-view tokens before destroying the exact bank.
  ~Impl(){match.reset();world.reset();if(staging)*staging=false;}
};
SourceMatchAuthority::Prepared::Prepared()=default;
SourceMatchAuthority::Prepared::~Prepared()=default;
SourceMatchAuthority::Prepared::Prepared(Prepared &&)noexcept=default;
SourceMatchAuthority::Prepared &SourceMatchAuthority::Prepared::operator=(Prepared &&)noexcept=default;
bool SourceMatchAuthority::Prepared::for_owner(const SourceMatchAuthority &owner)const noexcept{return impl_&&impl_->owner==&owner;}
void SourceMatchAuthority::Prepared::retain_live_multiplayer_render_history() {
  if(!impl_ || !impl_->world)throw FormatError("Native match render retention has no prepared world");
  impl_->world->retain_live_multiplayer_render_history();
}
void SourceMatchAuthority::Prepared::retain_live_multiplayer_render_history_from(
    const SourceMatchAuthority &source) {
  if(!impl_ || !impl_->world || impl_->owner==&source)
    throw FormatError("Native presentation history requires a distinct prepared Match owner");
  source.calls_.capture();source.match_.authority_calls_.capture();
  impl_->world->retain_live_multiplayer_render_history_from(source.world_);
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare(const Checkpoint &saved,AuthorityDigest binding) {
  return prepare(saved,binding,{});
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare(const Checkpoint &saved,AuthorityDigest binding,
    std::span<const SourcePlayerAdmission> roster) {
  calls_.prepare();
  if(saved.version!=2)throw FormatError("Unknown coupled TU1.5 authority checkpoint");
  // Service bindings are process-local and cannot be repaired by importing
  // native storage. Reject a missing owner before staging that storage.
  if(saved.match.service_roles!=match_.authority_service_roles())
    throw FormatError("Coupled source authority has incompatible local service roles");
  Prepared p;p.impl_=std::make_unique<Prepared::Impl>();auto &n=*p.impl_;n.owner=this;
  n.staging=&calls_.staging;calls_.staging=true;
  n.world.emplace(world_.prepare_authority(saved.world));
  validate_pair(saved.match,n.world->replacement_memory(),match_,match_.first_person_players_);
  if(!roster.empty())validate_source_players(saved.match,n.world->replacement_memory(),roster,true);
  const auto native_caller=n.world->replacement_caller();
  n.match.emplace(match_.prepare_authority(saved.match,binding,&n.world->replacement_memory(),&native_caller));return p;
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare_with_local_views(const Checkpoint &saved,
    AuthorityDigest binding,std::span<const LocalPlayerAdmission> admitted,SourceMatchAuthority &workspace) {
  return prepare_with_local_views(saved,binding,admitted,workspace,{});
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare_with_local_views(const Checkpoint &saved,
    AuthorityDigest binding,std::span<const LocalPlayerAdmission> admitted,SourceMatchAuthority &workspace,
    std::span<const SourcePlayerAdmission> roster,std::span<const unsigned> producer_views) {
  if(&workspace==this||&workspace.match_==&match_||&workspace.world_==&world_)
    throw FormatError("Reusable source workspace must be a separate private executor");
  if(producer_views.size()>4||roster.empty())
    throw FormatError("Reusable source workspace requires independently admitted producer views");
  std::uint16_t seen{};
  for(auto slot:producer_views) {
    if(slot>=multiplayer_capacity||(seen&(1u<<slot))||std::ranges::find(roster,slot,&SourcePlayerAdmission::slot)==roster.end())
      throw FormatError("Reusable source workspace has an invalid or unadmitted producer view");
    seen|=std::uint16_t(1u<<slot);
  }
  world_.require_authority_executor();workspace.world_.require_authority_executor();
  calls_.prepare();workspace.calls_.prepare();workspace.match_.authority_calls_.prepare();
  // No source calls run against the old recipient-mapped bank. Only change
  // the local definition selector used to validate the incoming complete cut;
  // its exact bank/view state then publishes through the ordinary transaction.
  // Failed imports remain unqualified for export and can be fully prepared
  // again. The live recipient is still untouched until its own final commit.
  workspace.calls_.fault=true;workspace.match_.authority_calls_.fault=true;
  workspace.match_.first_person_players_.assign(producer_views.begin(),producer_views.end());
  return prepare_with_local_views(saved,binding,admitted,workspace,roster);
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare_with_local_views(const Checkpoint &saved,
    AuthorityDigest binding,std::span<const LocalPlayerAdmission> admitted,SourceMatchAuthority &workspace,
    std::span<const SourcePlayerAdmission> roster) {
  calls_.prepare();
  if(&workspace==this||&workspace.match_==&match_||&workspace.world_==&world_)
    throw FormatError("Local-view authority preparation requires a separate disposable executor");
  Checkpoint rebound;
  {
    NativeAuthorityBoundary::Call active(calls_);
    match_.authority_calls_.prepare();workspace.calls_.prepare();
    const auto locals=match_.first_person_players_;
    std::vector<FirstPersonBindingDefinition> definitions;
    for(const auto &[key,definition]:match_.first_person_definitions_){(void)key;definitions.push_back(definition);}
    if(locals.empty()||locals.size()>4||admitted.size()!=locals.size())
      throw FormatError("Replacement authority requires its admitted local player views");
    for(unsigned view=0;view<locals.size();++view) {
      const auto id=locals[view];const auto &admission=admitted[view];
      const auto &incoming=saved.match.players.at(id),&current=match_.players_.at(id);
      if(!incoming.connected||!current.connected||(incoming.source_flags&2u)||(current.source_flags&2u)||
          incoming.source_datum==null_datum||admission.slot!=id||incoming.source_datum!=admission.player)
        throw FormatError("Replacement local view has a missing, departed or stale player identity");
    }
    auto staged=workspace.prepare(saved,binding);
    if(!workspace.commit(std::move(staged)))
      throw FormatError("Disposable local-view executor could not restore its admitted world");
    // Full native salts also need the source's actual identity bytes. A slot
    // or a chosen screen name alone cannot select another local player.
    const auto &incoming_bank=workspace.world_.memory();
    validate_source_players(saved.match,incoming_bank,admitted,false);
    if(!roster.empty())validate_source_players(saved.match,incoming_bank,roster,true);
    workspace.match_.configure_local_views(locals,definitions);
    rebound=workspace.checkpoint(binding);
  }
  return prepare(rebound,binding);
}
void SourceMatchAuthority::import_presentation(const Checkpoint &saved,AuthorityDigest binding,
    std::span<const LocalPlayerAdmission> admitted,std::span<const SourcePlayerAdmission> roster,
    std::span<const unsigned> producer_views,SourceMatchAuthority &displayed) {
  if(&displayed==this || &displayed.match_==&match_ || &displayed.world_==&world_)
    throw FormatError("Native presentation staging must be a distinct private world");
  calls_.prepare();match_.authority_calls_.prepare();
  displayed.calls_.capture();displayed.match_.authority_calls_.capture();
  world_.require_authority_executor();displayed.world_.require_authority_executor();
  displayed.world_.tu15_world().require_presentation_boundary();
  if(source_execution_inputs(world_.memory()) || source_execution_inputs(displayed.world_.memory()))
    throw FormatError("Private presentation import cannot consume a recovery execution journal");
  if(producer_views.size()>4 || roster.empty())
    throw FormatError("Native presentation requires independently admitted producer views and roster");
  std::uint16_t seen{};
  for(auto slot:producer_views) {
    if(slot>=multiplayer_capacity || (seen&(1u<<slot)) ||
        std::ranges::find(roster,slot,&SourcePlayerAdmission::slot)==roster.end())
      throw FormatError("Native presentation producer view has no unique admitted owner");
    seen|=std::uint16_t(1u<<slot);
  }
  const auto locals=displayed.match_.first_person_players_;
  std::vector<FirstPersonBindingDefinition> definitions;
  for(const auto &[key,definition]:displayed.match_.first_person_definitions_){(void)key;definitions.push_back(definition);}
  if(locals.empty() || locals.size()>4 || admitted.size()!=locals.size())
    throw FormatError("Native presentation requires its displayed admitted recipient views");
  for(unsigned view=0;view<locals.size();++view) {
    const auto id=locals[view];const auto &admission=admitted[view];
    const auto &incoming=saved.match.players.at(id),&current=displayed.match_.players_.at(id);
    if(!incoming.connected || !current.connected || (incoming.source_flags&2u) || (current.source_flags&2u) ||
        incoming.source_datum==null_datum || admission.slot!=id || incoming.source_datum!=admission.player ||
        std::ranges::find(roster,admission)==roster.end())
      throw FormatError("Native presentation recipient has a missing, departed or stale admitted identity");
  }
  // This unpublished owner alone changes its validation selector. The whole
  // ordinary native import still checks every source/resource/platform owner;
  // the displayed bank and its caller leases remain untouched on failure.
  calls_.fault=true;match_.authority_calls_.fault=true;
  match_.first_person_players_.assign(producer_views.begin(),producer_views.end());
  auto prepared=prepare(saved,binding,roster);
  prepared.retain_live_multiplayer_render_history_from(displayed);
  if(!commit(std::move(prepared)))
    throw FormatError("Native presentation staging could not commit its complete admitted world");
  validate_source_players(saved.match,world_.memory(),admitted,false);
  match_.configure_local_views(locals,definitions);
  const auto completed=execution_boundary();
  if(completed.tick!=saved.match.simulation.ticks)
    throw FormatError("Native presentation rebind changed its complete source tick");
  world_.tu15_world().require_presentation_boundary();
}
bool SourceMatchAuthority::commit(Prepared &&p)noexcept {
  if(!p.for_owner(*this))return false;
  if(!world_.authority_executor_thread())return false;
  if(!calls_.staging||calls_.depth)return false;
  auto &n=*p.impl_;
  // A fallible local wall-clock provider must be sampled before either bank
  // publishes. The following Match commit consumes this staged adjustment.
  if(!match_.prepare_result_clock(*n.match))return false;
  if(!world_.commit_authority(std::move(*n.world)))return false;
  calls_.fault=true;
  if(!match_.commit_authority(std::move(*n.match)))return false;
  calls_.fault=false;p.impl_.reset();return true;
}
}
