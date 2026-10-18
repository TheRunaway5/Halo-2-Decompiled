#include "halo2/multiplayer_authority.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "multiplayer_authority_fields.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>
#include <set>
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
}
}
std::uint32_t MultiplayerMatch::authority_service_roles()const {
    std::uint32_t result=0;unsigned i=0;
    for(bool present:{bool(source_object_world_),bool(king_services_.record),bool(king_services_.score_changed),bool(king_services_.notification),
        bool(first_person_event_owner_),bool(weapon_effect_owner_),bool(result_wall_clock_),bool(game_engine_presentation_owner_),
        bool(player_birth_owner_),bool(pre_player_update_owner_),
        source_object_world_&&source_object_world_->has_projectile_effect_observer()}){if(present)result|=1u<<i;++i;}
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
    std::optional<std::uint32_t> result_clock_adjustment;
    ~Impl(){if(staging)*staging=false;}
};
MultiplayerMatch::PreparedAuthority::PreparedAuthority()=default;
MultiplayerMatch::PreparedAuthority::~PreparedAuthority()=default;
MultiplayerMatch::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
MultiplayerMatch::PreparedAuthority &MultiplayerMatch::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool MultiplayerMatch::PreparedAuthority::for_owner(const MultiplayerMatch &m)const noexcept{return impl_&&impl_->owner==&m;}
MultiplayerMatch::PreparedAuthority MultiplayerMatch::prepare_authority(const Saved &saved,AuthorityDigest binding,const SourceRuntimeMemory *replacement) {
    authority_calls_.prepare();
    if(saved.version!=2||saved.admitted_binding!=binding||saved.simulation.admitted_binding!=binding||
       saved.configuration_binding!=authority_configuration()||saved.service_roles!=authority_service_roles()||
       saved.source_backed!=bool(player_pool_.source_memory())||saved.source_backed!=saved.source_players.has_value()||
       saved.source_backed==saved.owned_players.has_value()||saved.source_backed!=bool(replacement)||
       saved.source_backed!=saved.simulation.source_backed)
        throw FormatError("Match continuation has incompatible admission, definitions or local service roles");
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
    for(const auto &[h,r]:saved.retained_units){Unit u=r.value;if(r.animation)u.custom_animation=std::make_shared<const AnimationClip>(*r.animation);
        n.retained_units.emplace(h,std::move(u));}
    n.simulation.emplace(simulation_.prepare_authority(saved.simulation,binding,replacement));return p;
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
    retained_units_.swap(n.retained_units);authority_calls_.fault=false;p.impl_.reset();return true;
}
}
