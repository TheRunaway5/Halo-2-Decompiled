#include "halo2/multiplayer_authority.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/native_lag_compensation.hpp"
#include "network/multiplayer_authority_fields.hpp"
#include "runtime/world_continuation_bindings.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <sstream>
#include <type_traits>

namespace halo2 {
namespace {
using Saved=MultiplayerMatch::AuthorityCheckpoint;
using namespace world_continuation;
bool handle(std::uint32_t h){return h!=null_datum&&(h>>16)&&(h&65535u)<2048;}
void validate(const Saved &s,const DatumPool &pool,unsigned maximum,const LevelContent &content,int game_type) {
    if(pool.capacity()!=int(multiplayer_capacity)||pool.stride()!=0x21c||
       s.simulation.actors.size()!=multiplayer_capacity-1||s.result_events.index>=1000||
       s.winner< -1||s.winner>=int(multiplayer_capacity)||s.winner_team< -1||s.winner_team>=8||
       s.results_finalized&&!s.finished||s.game_over.active!=s.finished||
       s.world_weapon_configured&&s.world_weapon_rows.size()!=content.netgame_equipment.size()||
       s.weapon_pickups_configured&&s.pickup_rows.size()!=content.netgame_equipment.size())
        throw FormatError("Match continuation has incompatible capacity, result or configuration state");
    const auto object=[&](std::uint32_t h,std::uint32_t definition,unsigned type) {
        const NativeObjectLifetime *owner=nullptr;
        if(s.simulation.owned_lifetimes&&s.simulation.owned_lifetimes->contains(h))owner=&s.simulation.owned_lifetimes->owner(h);
        if(s.simulation.source_lifetimes){const auto at=s.simulation.source_lifetimes->owners.find(h);
            if(at!=s.simulation.source_lifetimes->owners.end())owner=&at->second;}
        if(!owner||!owner->published||owner->handle!=h||owner->definition!=definition||owner->type!=type)
            throw FormatError("Match continuation lost a published full object generation");
    };
    std::set<std::uint32_t> allocated;
    for(unsigned i=0;i<multiplayer_capacity;++i) {
        const auto &p=s.players[i];const auto &u=(i?s.simulation.actors[i-1]:s.simulation.player).value;
        validate_source_hud_messages(p.source_messages);
        if(p.source_datum!=null_datum) {
            if(!(p.source_datum>>16)||(p.source_datum&65535u)!=i||pool.lookup(p.source_datum).empty()||
               !allocated.insert(p.source_datum).second)
                throw FormatError("Match continuation lost its full player allocation identity");
        }
        if(p.connected&&(i>=maximum||p.source_datum==null_datum||p.source_membership<0||p.source_membership>15))
            throw FormatError("Match continuation has an unallocated active player");
        if(p.source_current_unit!=(p.source_current_object!=null_datum)||
           (p.source_current_unit&&(!p.connected||!u.exists||u.source_datum!=p.source_current_object||u.source_player!=p.source_datum)))
            throw FormatError("Match continuation changed the current player/unit binding");
        if(p.source_configuration)multiplayer_biped(content,source_session_player_representation(p.source_configuration,game_type));
        if(p.source_current_unit)object(u.source_datum,u.definition.datum,0);
    }
    if(allocated.size()!=std::size_t(pool.count()))throw FormatError("Match continuation omitted an allocated player record");
    for(const auto &[h,r]:s.retained_units) {
        const auto &u=r.value;
        if(h!=u.source_datum||!handle(h)||u.custom_animation||!finite(u.body.position)||!finite(u.body.velocity))
            throw FormatError("Match continuation changed retained unit ownership");
        auto definition=u.definition;const BipedDefinition *bound=nullptr;
        if(definition.datum==content.player.datum)bound=&content.player;
        for(const auto &p:content.player_representations)if(definition.datum==p.biped.datum)bound=&p.biped;
        if(!bound)throw FormatError("Match continuation has an unknown retained unit definition");
        definition.vitality.health=bound->vitality.health;definition.vitality.shield=bound->vitality.shield;
        if(biped_definition(definition)!=biped_definition(*bound)||!std::isfinite(u.definition.vitality.health)||!std::isfinite(u.definition.vitality.shield))
            throw FormatError("Match continuation changed retained unit operands");
        object(h,u.definition.datum,0);
    }
    for(const auto &[h,c]:s.corpses) {
        bool unit=s.retained_units.contains(h)||s.simulation.player.value.source_datum==h;
        for(const auto &r:s.simulation.actors)unit|=r.value.source_datum==h;
        if(h!=c.object||!handle(h)||!unit||!finite(c.velocity))
            throw FormatError("Match continuation lost its retained corpse owner");
    }
    for(const auto &p:s.weapon_pickups)if(!handle(p.handle)||!finite(p.position)||!finite(p.forward)||!finite(p.up)||
        !content.weapons.contains(p.definition)||(p.weapon&&(p.weapon->source_datum!=p.handle||p.weapon->definition!=p.definition)))
        throw FormatError("Match continuation changed a live pickup generation or definition");
    for(const auto &p:s.weapon_pickups)object(p.handle,p.definition,2);
    if(s.defusal) {
        validate_bomb_defusal_state(*s.defusal);
        if(!content.weapons.contains(s.defusal_definition))throw FormatError("Defusal continuation lost its bomb definition");
        object(s.defusal_bomb,s.defusal_definition,2);
    }
    const auto view=[](const FirstPersonViewBinding &v){for(const auto &h:v.hands)
        if(h.weapon_node_count>h.weapon_nodes.size()||h.arms_node_count>h.arms_nodes.size()||!std::isfinite(h.rate)||!std::isfinite(h.frame))
            throw FormatError("Match continuation has invalid first-person channel state");};
    for(const auto &v:s.first_person_views)view(v);for(const auto &v:s.first_person_clock_origins)view(v);
    for(const auto &v:s.replicated_fp_views)view(v);for(const auto &v:s.replicated_fp_origins)view(v);
    std::uint64_t last=0;
    for(const auto &e:s.rumble_events){if(e.serial<=last||e.serial>s.rumble_serial||e.tick>s.simulation.ticks)
        throw FormatError("Match continuation changed rumble journal ordering");last=e.serial;}
    std::uint32_t serial=0;
    for(const auto &e:s.weapon_effect_journal){if(e.serial<=serial||e.serial>s.weapon_effect_serial||e.tick>s.simulation.ticks||!finite(e.velocity))
        throw FormatError("Match continuation changed weapon effect journal ordering");serial=e.serial;}
    validate_physics_impulse_journal(s.physics_impulses,std::uint32_t(s.simulation.ticks));
    if(!s.physics_impulses.empty()&&std::bit_cast<std::int32_t>(s.physics_impulses.back().serial-s.physics_impulse_serial)>0)
        throw FormatError("Match continuation changed physics impulse journal ordering");
    // The rewind owner's own staging validates the frames against its window.
    if(std::uint8_t(s.netcode_mode)>std::uint8_t(NetcodeMode::full)||
       (s.lag_compensation&&(s.netcode_mode==NetcodeMode::original||s.lag_compensation->mode!=s.netcode_mode)))
        throw FormatError("Match continuation has an invalid netcode mode or rewind history");
}
}
std::uint32_t MultiplayerMatch::authority_service_roles()const {
    std::uint32_t result=0;unsigned i=0;
    for(bool present:{bool(source_object_world_),bool(king_services_.record),bool(king_services_.score_changed),bool(king_services_.notification),
        bool(first_person_event_owner_),bool(weapon_effect_owner_),bool(result_wall_clock_),bool(game_engine_presentation_owner_),
        bool(player_birth_owner_),bool(pre_player_update_owner_),
        source_object_world_&&source_object_world_->has_projectile_effect_observer(),
        bool(native_first_person_registry_)}){if(present)result|=1u<<i;++i;}
    return result;
}
MultiplayerMatch::AuthorityCheckpoint MultiplayerMatch::authority_checkpoint(AuthorityDigest binding,bool qualified_source_world)const {
    authority_calls_.capture();
    Saved saved;saved.admitted_binding=binding;saved.configuration_binding=authority_configuration();
    saved.service_roles=authority_service_roles();saved.source_backed=player_pool_.source_memory()!=nullptr;
    saved.logical_result_seconds=result_wall_time();
    if(saved.source_backed&&!qualified_source_world)throw FormatError("Match continuation requires its complete qualified source world");
    if(saved.source_backed)saved.source_players=player_pool_.source_binding();else saved.owned_players=player_pool_;
    saved.simulation=simulation_.authority_checkpoint(binding,qualified_source_world);
    if(native_first_person_registry_)saved.native_first_person=native_first_person_registry_->checkpoint();
    saved.netcode_mode=netcode_mode_;
    if(lag_compensation_)saved.lag_compensation=lag_compensation_->history();
#define SAVE(name) saved.name=name##_;
    HALO2_MULTIPLAYER_AUTHORITY_VALUES(SAVE)
#undef SAVE
    for(const auto &[h,u]:retained_units_){Simulation::AuthorityCheckpoint::UnitRecord r;r.value=u;
        if(u.custom_animation)r.animation=*u.custom_animation;r.value.custom_animation.reset();saved.retained_units.emplace(h,std::move(r));}
    validate(saved,player_pool_,rules_.maximum_players,content_,source_game_context_.game_type);return saved;
}
struct MultiplayerMatch::PreparedAuthority::Impl {
    MultiplayerMatch *owner{};bool *staging{};
    Saved values;
    std::map<std::uint32_t,Unit> retained_units;
    std::optional<DatumPool::PreparedSourceView> source_players;
    std::optional<Simulation::PreparedAuthority> simulation;
    std::unique_ptr<NativeFirstPersonRegistryTu15> native_first_person;
    std::optional<std::uint32_t> result_clock_adjustment;
    // A detached replacement rewind owner (the cut's mode differs from this
    // world's owner, or this world has none), and the staged history.
    std::unique_ptr<NativeLagCompensation> lag_owner;
    NativeLagCompensation::StagedHistory lag_history;
    bool lag_replace{};
    ~Impl(){if(staging)*staging=false;}
};
MultiplayerMatch::PreparedAuthority::PreparedAuthority()=default;
MultiplayerMatch::PreparedAuthority::~PreparedAuthority()=default;
MultiplayerMatch::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
MultiplayerMatch::PreparedAuthority &MultiplayerMatch::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool MultiplayerMatch::PreparedAuthority::for_owner(const MultiplayerMatch &m)const noexcept{return impl_&&impl_->owner==&m;}
MultiplayerMatch::PreparedAuthority MultiplayerMatch::prepare_authority(const Saved &saved,AuthorityDigest binding,
    const SourceRuntimeMemory *replacement,const SourcePhysicsWorldRuntimeAbi *native_caller) {
    authority_calls_.prepare();
    const char *first_failed{};std::uint32_t current_service_roles{};bool observed_source_backing{};
    const auto incompatible=[&](const char *name,bool failed)noexcept{
        if(failed)first_failed=name;return failed;
    };
    if(incompatible("version",saved.version!=3)||incompatible("admitted_binding",saved.admitted_binding!=binding)||
       incompatible("simulation.admitted_binding",saved.simulation.admitted_binding!=binding)||
       incompatible("configuration_binding",saved.configuration_binding!=authority_configuration())||
       incompatible("service_roles",saved.service_roles!=(current_service_roles=authority_service_roles()))||
       incompatible("player_pool.source_memory",saved.source_backed!=(observed_source_backing=bool(player_pool_.source_memory())))||
       incompatible("source_players",saved.source_backed!=(observed_source_backing=saved.source_players.has_value()))||
       incompatible("owned_players",saved.source_backed==(observed_source_backing=saved.owned_players.has_value()))||
       incompatible("replacement",saved.source_backed!=(observed_source_backing=bool(replacement)))||
       incompatible("simulation.source_backed",saved.source_backed!=(observed_source_backing=saved.simulation.source_backed))) {
        std::ostringstream message;message<<"Match continuation has incompatible admission, definitions or local service roles"
            <<"; first_failed="<<first_failed;
        if(std::string_view(first_failed)=="version")message<<" saved_version="<<saved.version<<" expected_version=3";
        else if(std::string_view(first_failed)=="service_roles")message<<" saved_roles="<<saved.service_roles
            <<" current_roles="<<current_service_roles<<" differing_roles="<<(saved.service_roles^current_service_roles);
        else if(std::string_view(first_failed)=="player_pool.source_memory"||std::string_view(first_failed)=="source_players"||
                std::string_view(first_failed)=="owned_players"||std::string_view(first_failed)=="replacement"||
                std::string_view(first_failed)=="simulation.source_backed")
            message<<" saved_source_backed="<<saved.source_backed<<" observed_source_backing="<<observed_source_backing;
        throw FormatError(message.str());
    }
    if(saved.source_gameplay_fade.has_value()!=source_engine_owned_)
        throw FormatError("Match continuation changed its native viewport-fade source owner");
    if(saved.native_first_person.has_value()!=bool(native_first_person_registry_))
        throw FormatError("Match continuation changed its native first-person Cache owner");
    if(saved.native_first_person && !native_caller)
        throw FormatError("Native FP continuation requires its staged native caller");
    if(saved.source_gameplay_fade)validate_source_gameplay_fade_journal_tu15(*saved.source_gameplay_fade);
    PreparedAuthority p;p.impl_=std::make_unique<PreparedAuthority::Impl>();auto &n=*p.impl_;n.owner=this;
    n.staging=&authority_calls_.staging;authority_calls_.staging=true;
    if(saved.source_backed) {
        n.source_players.emplace(player_pool_.prepare_source_view(*saved.source_players,*replacement));
        // Read-only view for validation; no allocation metadata is imported by
        // this temporary wrapper. The exact bank has already been staged.
        DatumPool view(*const_cast<SourceRuntimeMemory *>(replacement),saved.source_players->pool);
        validate(saved,view,rules_.maximum_players,content_,source_game_context_.game_type);
    }else {
        if(saved.owned_players->source_memory())throw FormatError("Match continuation imported a process player-pool pointer");
        validate(saved,*saved.owned_players,rules_.maximum_players,content_,source_game_context_.game_type);
    }
    n.values=saved;
    // Mirror the producer's rewind owner: keep this owner when the mode
    // matches, build a detached one when it differs or is missing, and drop
    // it for a cut without one. The history is validated and staged now.
    if(saved.lag_compensation) {
        if(!netcode_capable_)throw FormatError("Lag-compensated continuation requires a host-capable TU1.5 world");
        if(!lag_compensation_||lag_compensation_->mode()!=saved.lag_compensation->mode) {
            n.lag_owner=make_lag_compensation(saved.lag_compensation->mode,false);n.lag_replace=true;
        }
        n.lag_history=(n.lag_owner?*n.lag_owner:*lag_compensation_).stage_history(*saved.lag_compensation,saved.simulation.ticks);
    }else n.lag_replace=bool(lag_compensation_);
    n.values.lag_compensation.reset();
    for(const auto &[h,r]:saved.retained_units){Unit u=r.value;if(r.animation)u.custom_animation=std::make_shared<const AnimationClip>(*r.animation);
        n.retained_units.emplace(h,std::move(u));}
    n.simulation.emplace(simulation_.prepare_authority(saved.simulation,binding,replacement));
    if(saved.native_first_person) {
        NativeFirstPersonRegistryTu15::Bindings bindings;
        for(unsigned slot=0;slot<bindings.size();++slot) {
            const auto &player=saved.players[slot];if(!player.connected || (player.source_flags&2u))continue;
            const auto representation=source_session_player_representation(
                player.source_configuration,source_game_context_.game_type);
            bindings[slot]={player.source_datum,multiplayer_biped(content_,representation).datum,
                int(representation),player.connected && !(player.source_flags&2u)};
        }
        n.native_first_person=native_first_person_registry_->prepare_restore(*replacement,*native_caller,*saved.native_first_person,
            bindings,std::uint32_t(saved.simulation.ticks));
    }
    return p;
}
bool MultiplayerMatch::prepare_result_clock(PreparedAuthority &p) const noexcept {
    if(!p.for_owner(*this)||!authority_calls_.staging||authority_calls_.depth)return false;
    try{p.impl_->result_clock_adjustment=p.impl_->values.logical_result_seconds-result_wall_clock_();return true;}
    catch(...){return false;}
}
bool MultiplayerMatch::commit_authority(PreparedAuthority &&p) noexcept {
    if(!p.for_owner(*this)||!authority_calls_.staging||authority_calls_.depth)return false;
    auto &n=*p.impl_;
    if(n.source_players&&!player_pool_.source_view_ready(*n.source_players))return false;
    if(!n.result_clock_adjustment&&!prepare_result_clock(p))return false;
    authority_calls_.fault=true;
    if(!simulation_.commit_authority(std::move(*n.simulation)))return false;
    if(n.source_players&&!player_pool_.commit_source_view(std::move(*n.source_players)))return false;
    if(n.values.owned_players){static_assert(std::is_nothrow_swappable_v<DatumPool>);std::swap(player_pool_,*n.values.owned_players);}
#define SWAP(name) static_assert(std::is_nothrow_swappable_v<decltype(name##_)>);std::swap(name##_,n.values.name);
    HALO2_MULTIPLAYER_AUTHORITY_VALUES(SWAP)
#undef SWAP
    result_wall_adjustment_=*n.result_clock_adjustment;
    retained_units_.swap(n.retained_units);native_first_person_registry_.swap(n.native_first_person);
    if(n.lag_replace) {
        // One owner registration replaces (or clears) the world's owner; it
        // allocates nothing. A refusal leaves this transaction faulted.
        try{if(n.lag_owner)n.lag_owner->attach();else lag_compensation_->detach();}catch(...){return false;}
        if(n.lag_owner&&lag_compensation_)lag_compensation_->displaced();
        lag_compensation_.swap(n.lag_owner);
    }
    if(lag_compensation_)lag_compensation_->adopt_history(n.lag_history);
    netcode_mode_=n.values.netcode_mode;
    authority_calls_.fault=false;p.impl_.reset();return true;
}
}
