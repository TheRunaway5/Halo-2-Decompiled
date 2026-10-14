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
void validate_pair(const MultiplayerMatch::AuthorityCheckpoint &match,const SourceRuntimeMemory &bank,
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
  if(!game||game>UINT32_MAX-0x26cu||word(bank,game+8u)!=2||word(bank,game+0xcu)!=(1u|(30u<<16))||
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
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare(const Checkpoint &saved,AuthorityDigest binding) {
  calls_.prepare();
  if(saved.version!=1)throw FormatError("Unknown coupled TU1.5 authority checkpoint");
  Prepared p;p.impl_=std::make_unique<Prepared::Impl>();auto &n=*p.impl_;n.owner=this;
  n.staging=&calls_.staging;calls_.staging=true;
  n.world.emplace(world_.prepare_authority(saved.world));
  validate_pair(saved.match,n.world->replacement_memory(),match_,match_.first_person_players_);
  n.match.emplace(match_.prepare_authority(saved.match,binding,&n.world->replacement_memory()));return p;
}
SourceMatchAuthority::Prepared SourceMatchAuthority::prepare_with_local_views(const Checkpoint &saved,
    AuthorityDigest binding,std::span<const LocalPlayerAdmission> admitted,SourceMatchAuthority &workspace) {
  return prepare_with_local_views(saved,binding,admitted,workspace,{});
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
    const SourceRawDatumPoolServices incoming_storage{.read=[&](auto at,auto n){return incoming_bank.view(at,n);}};
    const auto verify_identity=[&](const auto &admission) {
      const auto full=admission.player;
      if(admission.slot>=multiplayer_capacity||saved.match.players[admission.slot].source_datum!=full)
        throw FormatError("Replacement source player differs from its independently accepted slot");
      const auto a=source_raw_datum_lookup(word(incoming_bank,0x4f54f4u),full,incoming_storage);
      if(!a||!std::ranges::equal(incoming_bank.view(a+4u,12),std::as_bytes(std::span(admission.identity))))
        throw FormatError("Replacement has a different full source player identity");
    };
    for(const auto &admission:admitted)verify_identity(admission);
    if(!roster.empty()) {
      if(roster.size()>multiplayer_capacity)throw FormatError("Replacement source roster exceeds native capacity");
      std::uint16_t allocated{};
      for(const auto &entry:roster){verify_identity(entry);if(allocated&(1u<<entry.slot))throw FormatError("Replacement source roster duplicates a player");allocated|=std::uint16_t(1u<<entry.slot);}
      for(unsigned slot=0;slot<multiplayer_capacity;++slot)
        if(bool(allocated&(1u<<slot))!=(saved.match.players[slot].source_datum!=null_datum))
          throw FormatError("Replacement source cut omitted or introduced an admitted player allocation");
    }
    workspace.match_.configure_local_views(locals,definitions);
    rebound=workspace.checkpoint(binding);
  }
  return prepare(rebound,binding);
}
bool SourceMatchAuthority::commit(Prepared &&p)noexcept {
  if(!p.for_owner(*this)||!calls_.staging||calls_.depth)return false;
  auto &n=*p.impl_;
  if(!world_.commit_authority(std::move(*n.world)))return false;
  calls_.fault=true;
  if(!match_.commit_authority(std::move(*n.match)))return false;
  calls_.fault=false;p.impl_.reset();return true;
}
}
