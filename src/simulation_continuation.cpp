#include "halo2/simulation_authority.hpp"
#include "simulation_world.hpp"
#include "simulation_authority_fields.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>
#include <bit>
#include <set>
#include <type_traits>

namespace halo2 {
namespace {
using Saved=Simulation::AuthorityCheckpoint;
using namespace world_continuation;
Saved::UnitRecord save_unit(const Unit &unit){
    Saved::UnitRecord saved;saved.value=unit;
    if(unit.custom_animation)saved.animation=*unit.custom_animation;
    saved.value.custom_animation.reset();return saved;
}
Unit load_unit(const Saved::UnitRecord &saved){
    if(saved.value.custom_animation)throw FormatError("Simulation checkpoint imported a process animation pointer");
    Unit unit=saved.value;if(saved.animation)unit.custom_animation=std::make_shared<const AnimationClip>(*saved.animation);return unit;
}
bool handle(std::uint32_t h){return h!=null_datum && (h>>16) && (h&65535u)<2048;}
void validate_unit(const Saved::UnitRecord &saved,const LevelContent &content,std::uint64_t tick){
    const auto &u=saved.value;
    if(u.custom_animation || (u.source_datum!=null_datum && !handle(u.source_datum)) ||
        !finite(u.body.position)||!finite(u.body.velocity)||!finite(u.body.support_normal)||
        !std::isfinite(u.body.height)||!std::isfinite(u.body.radius)||!std::isfinite(u.body.maximum_slope_cosine)||
        !std::isfinite(u.body.landing_speed)||!std::isfinite(u.pitch)||!std::isfinite(u.yaw)||!std::isfinite(u.crouch)||
        !std::isfinite(u.awareness)||!std::isfinite(u.vitality.body)||!std::isfinite(u.vitality.shield)||
        !std::isfinite(u.definition.vitality.health)||!std::isfinite(u.definition.vitality.shield)||
        !finite(u.seat_action.entry_position)||
        (u.seat_action.phase!=Unit::SeatAction::None&&u.seat_action.started>tick)||
        u.state<Unit::Idle||u.state>Unit::Dead||u.seat_action.phase<Unit::SeatAction::None||u.seat_action.phase>Unit::SeatAction::Exiting||
        (u.body.support_triangle && *u.body.support_triangle>=UINT32_MAX)||u.waypoint>u.route.size()||
        u.bsp< -1||(u.bsp>=0&&std::size_t(u.bsp)>=content.bsp_count))
        throw FormatError("Simulation continuation has invalid unit identity/body/control state");
    // Inactive/hidden bodies can retain contact with a previous collision view.
    // The native producer's permutation is uint32; move() resets the contact
    // before querying a new view. Do not rewrite/reject that retained state
    // merely because current scene deletion/BSP changes reduced its geometry.
    const auto matches=[&](const BipedDefinition &d){
        // Existing unit_set_maximum_vitality changes these two per-unit values.
        // Preserve them as state while binding every other definition operand.
        auto authored=u.definition;authored.vitality.health=d.vitality.health;authored.vitality.shield=d.vitality.shield;
        return d.datum==u.definition.datum&&biped_definition(d)==biped_definition(authored);
    };
    bool bound=matches(content.player);
    for(const auto &r:content.player_representations)bound|=matches(r.biped);
    for(const auto &[id,c]:content.characters){(void)id;bound|=matches(c.biped);}
    // Match owns fifteen initially unallocated native slots. They have no
    // biped definition until an actual spawn; do not invent one on import.
    const bool cold=!u.exists && u.source_datum==null_datum && u.definition.datum==null_datum;
    if(!bound&&!cold)throw FormatError("Simulation continuation unit definition differs from local content");
    for(auto p:u.route)if(!finite(p))throw FormatError("Simulation continuation has invalid unit route");
    for(const auto &p:{u.command.goal,u.command.face_point,u.command.look_point})
        if(p&&!finite(*p))throw FormatError("Simulation continuation has invalid command point");
    for(auto index:u.source_inventory.current)if(index< -1||index>=4)throw FormatError("Simulation continuation has invalid inventory hand");
    for(auto index:u.source_inventory.requested)if(index< -1||index>=4)throw FormatError("Simulation continuation has invalid inventory request");
    if(u.source_inventory.current[0]>=0&&u.weapon.source_datum!=u.source_inventory.slots[unsigned(u.source_inventory.current[0])])
        throw FormatError("Simulation continuation changed its active inventory identity");
    for(unsigned i=0;i<4;++i){const auto &w=u.source_inventory.inactive_weapons[i];
        if(w && (u.source_inventory.current[0]==int(i) || w->source_datum!=u.source_inventory.slots[i]))
            throw FormatError("Simulation continuation duplicated its active ammo owner or changed inventory identity");}
}
void validate_values(const Saved &saved,const LevelContent &content){
    if(saved.difficulty>3||saved.bsp>=content.bsp_count||(saved.pending_bsp&&*saved.pending_bsp>=content.bsp_count)||
        saved.fixed_collision.has_value()!=saved.all_collision.has_value()||
        saved.actors.size()>256||saved.vehicle_rows.size()!=content.netgame_equipment.size()||
        !std::isfinite(saved.player_pitch)||saved.script_lists.size()>65536||saved.next_script_list==null_datum)
        throw FormatError("Simulation continuation changes native capacity/clock/configuration");
    const auto &bsp=saved.bsp_geometry.collision;
    const auto prefix=[](std::span<const CollisionTriangle> base,std::span<const CollisionTriangle> all){
        return all.size()>=base.size()&&triangles(base)==triangles(all.first(base.size()));};
    if(triangles(bsp)!=triangles(saved.bsp_collision.triangles)||
        (saved.fixed_collision&&!prefix(bsp,saved.fixed_collision->triangles))||
        (saved.all_collision&&!prefix(saved.fixed_collision?std::span(saved.fixed_collision->triangles):std::span(bsp),saved.all_collision->triangles)))
        throw FormatError("Simulation continuation changed collision prefix/contact indices");
    validate_unit(saved.player,content,saved.ticks);
    for(const auto &unit:saved.actors)validate_unit(unit,content,saved.ticks);
    for(const auto &flip:saved.vehicle_flips)
        if(!handle(flip.object)||flip.direction<1||flip.direction>4||flip.ticks>60||!finite(flip.velocity))
            throw FormatError("Simulation continuation has invalid vehicle flip state");
    const auto geometry=[](std::span<const CollisionTriangle> values){for(const auto &t:values)
        if(!finite(t.a)||!finite(t.b)||!finite(t.c)||!finite(t.source_normal))
            throw FormatError("Simulation continuation has nonfinite cached collision");};
    geometry(bsp);if(saved.fixed_collision)geometry(saved.fixed_collision->triangles);if(saved.all_collision)geometry(saved.all_collision->triangles);
    for(const auto &[h,list]:saved.script_lists){(void)list;if(h<0x80000001u||h>=saved.next_script_list)
        throw FormatError("Simulation continuation has an invalid native script list identity");}
    validate_source_device_events(saved.device_events,saved.ticks);
    if((!saved.device_events.empty()&&saved.device_events.back().serial!=saved.device_event_serial) ||
        (saved.device_events.empty()&&saved.device_event_serial))
        throw FormatError("Simulation continuation lost its device journal frontier");
    for(const auto &p:saved.projectiles){
        const auto w=content.weapons.find(p.weapon);
        if(!finite(p.position)||!finite(p.velocity)||!finite(p.forward)||!finite(p.up)||!std::isfinite(p.traveled)||
            w==content.weapons.end()||(p.source_datum!=null_datum&&!handle(p.source_datum))||p.barrel_index< -1||
            (p.barrel_index>=0&&(std::size_t(p.barrel_index)>=w->second.barrel_views.size()||!w->second.barrel_views[unsigned(p.barrel_index)])))
            throw FormatError("Simulation continuation has invalid weapon projectile state/birth barrel");
    }
    for(const auto &[h,a]:saved.object_animations){(void)a;if(!saved.scene.slots.contains(h))
        throw FormatError("Simulation continuation has an orphaned object animation");}
}
}
std::uint32_t Simulation::authority_service_roles()const {
    std::uint32_t roles=0;unsigned i=0;
    for(bool present:{bool(bsp_loader_),bool(predictor_),bool(vehicle_allied_),bool(subtitle_localized_),bool(sound_duration_),
        bool(animation_loader_),bool(look_invert_get_),bool(look_invert_set_),bool(source_vehicle_seat_query_),
        bool(allocate_object_),bool(publish_object_),bool(release_object_),bool(damage_hook_),bool(player_damage_scale_),
        bool(first_person_activity_hook_),bool(first_person_timing_hook_),bool(weapon_effect_hook_),bool(world_weapon_effect_hook_),
        bool(grenade_sandbox_),bool(melee_animation_),bool(source_device_script_)}){if(present)roles|=1u<<i;++i;}
    return roles;
}
Simulation::AuthorityCheckpoint Simulation::authority_checkpoint(AuthorityDigest admitted_binding,bool qualified_source_world)const {
    authority_calls_.capture();
    if(checkpoint_||!debug_snapshots_.empty()||save_pending_||revert_pending_)
        throw FormatError("ONLINE simulation continuation cannot omit campaign/debug saved-state owners");
    const bool source=object_lifetimes_.pool().source_memory()!=nullptr;
    if((source||source_physics_movement_)&&!qualified_source_world)
        throw FormatError("Simulation continuation requires the enclosing qualified complete source world");
    Saved saved;saved.admitted_binding=admitted_binding;saved.service_roles=authority_service_roles();saved.source_backed=source;
    saved.unit_interaction_binding=unit_interaction_definitions(content_);
#define SAVE(name) saved.name=name##_;
    HALO2_SIMULATION_AUTHORITY_VALUES(SAVE)
#undef SAVE
    if(source)saved.source_lifetimes=object_lifetimes_.source_authority_checkpoint();else saved.owned_lifetimes=object_lifetimes_;
    saved.player=save_unit(player_);for(const auto &u:actors_)saved.actors.push_back(save_unit(u));
    for(const auto &[h,a]:object_animations_){Saved::AnimationRecord value;if(a.clip)value.clip=*a.clip;value.tick=a.tick;value.loop=a.loop;
        saved.object_animations.emplace(h,std::move(value));}
    saved.bsp_geometry.navigation=*world_->sectors;
    saved.bsp_geometry.collision.assign(world_->collision->triangles().begin(),world_->collision->triangles().end());
    saved.bsp_binding=world_continuation::bsp_geometry(saved.bsp_geometry);
    saved.bsp_collision=world_->collision->authority_checkpoint();
    if(static_collision_)saved.fixed_collision=static_collision_->authority_checkpoint();
    if(prop_collision_)saved.all_collision=prop_collision_->authority_checkpoint();
    saved.scene=scene_.authority_checkpoint();saved.devices=devices_.authority_checkpoint(ticks_);
    saved.props=props_.authority_checkpoint(saved.scene);saved.scenery=scenery_collision_.authority_checkpoint(saved.scene);
    saved.presentation=presentation_.authority_checkpoint();saved.scripts=scripts_->authority_checkpoint();
    if(grenade_sandbox_){std::vector<Unit> units;units.push_back(player_);units.insert(units.end(),actors_.begin(),actors_.end());
        saved.grenades=grenade_sandbox_->authority_checkpoint(units,qualified_source_world);}
    if(melee_animation_)saved.melee_binding=melee_animation_->authority_binding();
    validate_values(saved,content_);return saved;
}
struct Simulation::PreparedAuthority::Impl {
    Simulation *owner{};bool *staging{};
    Saved values;
    Unit player;std::vector<Unit> actors;
    std::map<std::uint32_t,ObjectAnimation> animations;
    std::shared_ptr<const BspWorld> world;
    std::shared_ptr<const CollisionWorld> fixed,all;
    std::optional<NativeObjectLifetimes::PreparedSourceAuthority> source_lifetimes;
    std::optional<SceneObjects::PreparedAuthority> scene;
    std::optional<SourceDeviceWorld::PreparedAuthority> devices;
    std::optional<PropSandbox::PreparedAuthority> props;
    std::optional<StaticSceneryCollision::PreparedAuthority> scenery;
    std::optional<Presentation::PreparedAuthority> presentation;
    std::optional<ScriptVm::PreparedAuthority> scripts;
    std::optional<NativeGrenadeSandbox::PreparedAuthority> grenades;
    std::shared_ptr<NativeMeleeAnimation> melee;
    ~Impl(){if(staging)*staging=false;}
};
Simulation::PreparedAuthority::PreparedAuthority()=default;
Simulation::PreparedAuthority::~PreparedAuthority()=default;
Simulation::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
Simulation::PreparedAuthority &Simulation::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool Simulation::PreparedAuthority::for_owner(const Simulation &owner)const noexcept{return impl_&&impl_->owner==&owner;}
Simulation::PreparedAuthority Simulation::prepare_authority(const Saved &saved,AuthorityDigest binding,
    const SourceRuntimeMemory *replacement) {
    authority_calls_.prepare();
    if(saved.version!=2||saved.admitted_binding!=binding||saved.service_roles!=authority_service_roles()||
        saved.unit_interaction_binding!=unit_interaction_definitions(content_)||
        saved.object_bodies.capacity()!=object_bodies_.capacity()||
        saved.source_backed!=bool(object_lifetimes_.pool().source_memory())||
        saved.multiplayer_weapon_steps!=multiplayer_weapon_steps_||saved.source_physics_movement!=source_physics_movement_||
        bool(saved.grenades)!=bool(grenade_sandbox_)||bool(saved.melee_binding)!=bool(melee_animation_)||
        checkpoint_||!debug_snapshots_.empty()||save_pending_||revert_pending_)
        throw FormatError("Simulation continuation has incompatible admission/services/owner roles");
    if(saved.source_backed!=saved.source_lifetimes.has_value()||saved.source_backed==saved.owned_lifetimes.has_value()||
        (saved.source_backed&&!replacement)||(!saved.source_backed&&replacement))
        throw FormatError("Simulation continuation lost its coupled identity/world owner");
    PreparedAuthority p;p.impl_=std::make_unique<PreparedAuthority::Impl>();p.impl_->owner=this;
    p.impl_->staging=&authority_calls_.staging;authority_calls_.staging=true;auto &n=*p.impl_;
    validate_values(saved,content_);
    BspContent local;
    if(saved.bsp==bsp_){local.navigation=*world_->sectors;local.collision.assign(world_->collision->triangles().begin(),world_->collision->triangles().end());}
    else {if(!bsp_loader_)throw FormatError("Simulation continuation has no local BSP loader");local=bsp_loader_(saved.bsp);}
    if(saved.bsp_binding!=world_continuation::bsp_geometry(saved.bsp_geometry)||saved.bsp_binding!=world_continuation::bsp_geometry(local))
        throw FormatError("Simulation continuation BSP/navigation differs from local admitted geometry");
    n.world=std::make_shared<BspWorld>(std::make_shared<const std::vector<NavigationSector>>(std::move(local.navigation)),
        CollisionWorld::restore_authority(saved.bsp_collision));
    if(saved.fixed_collision)n.fixed=CollisionWorld::restore_authority(*saved.fixed_collision);
    if(saved.all_collision)n.all=CollisionWorld::restore_authority(*saved.all_collision);
    n.values=saved;n.player=load_unit(saved.player);for(const auto &u:saved.actors)n.actors.push_back(load_unit(u));
    for(const auto &[h,a]:saved.object_animations){ObjectAnimation value;if(a.clip)value.clip=std::make_shared<const AnimationClip>(*a.clip);
        value.tick=a.tick;value.loop=a.loop;n.animations.emplace(h,std::move(value));}
    if(saved.source_backed){n.source_lifetimes.emplace(object_lifetimes_.prepare_source_authority(*saved.source_lifetimes,*replacement));
        n.scene.emplace(scene_.prepare_authority(saved.scene,*saved.source_lifetimes,*replacement));}
    else {if(saved.owned_lifetimes->pool().source_memory())throw FormatError("Simulation continuation imported a source process pointer");
        n.scene.emplace(scene_.prepare_authority(saved.scene,*saved.owned_lifetimes));}
    n.devices.emplace(devices_.prepare_authority(saved.devices,saved.ticks));
    n.props.emplace(props_.prepare_authority(saved.props,saved.scene));n.scenery.emplace(scenery_collision_.prepare_authority(saved.scenery,saved.scene));
    n.presentation.emplace(presentation_.prepare_authority(saved.presentation));n.scripts.emplace(scripts_->prepare_authority(saved.scripts));
    if(saved.grenades){std::vector<Unit> units;units.push_back(n.player);units.insert(units.end(),n.actors.begin(),n.actors.end());
        n.grenades.emplace(grenade_sandbox_->prepare_authority(*saved.grenades,units,replacement!=nullptr));}
    if(saved.melee_binding)n.melee=melee_animation_->prepare_authority_owner(content_,*saved.melee_binding);
    return p;
}
bool Simulation::commit_authority(PreparedAuthority &&p) noexcept {
    if(!p.for_owner(*this)||!authority_calls_.staging||authority_calls_.depth)return false;
    auto &n=*p.impl_;
    // A source world must publish its exact staged backing first. A premature
    // call leaves this transaction and all native banks unchanged.
    if(n.source_lifetimes&&!object_lifetimes_.commit_source_authority(std::move(*n.source_lifetimes)))return false;
    if(n.values.owned_lifetimes){static_assert(std::is_nothrow_swappable_v<NativeObjectLifetimes>);
        std::swap(object_lifetimes_,*n.values.owned_lifetimes);}
    // All these tokens were admitted together and their owners are staged.
    // Failure means the enclosing barrier was violated; never resume/export a
    // partially published root as a completed world.
    authority_calls_.fault=true;
    if(!scene_.commit_authority(std::move(*n.scene))||!devices_.commit_authority(std::move(*n.devices))||
        !props_.commit_authority(std::move(*n.props))||!scenery_collision_.commit_authority(std::move(*n.scenery))||
        !presentation_.commit_authority(std::move(*n.presentation))||!scripts_->commit_authority(std::move(*n.scripts))||
        (n.grenades&&!grenade_sandbox_->commit_authority(std::move(*n.grenades))))return false;
#define SWAP(name) static_assert(std::is_nothrow_swappable_v<decltype(name##_)>);std::swap(name##_,n.values.name);
    HALO2_SIMULATION_AUTHORITY_VALUES(SWAP)
#undef SWAP
    static_assert(std::is_nothrow_swappable_v<Unit>);
    std::swap(player_,n.player);actors_.swap(n.actors);object_animations_.swap(n.animations);
    world_.swap(n.world);static_collision_.swap(n.fixed);prop_collision_.swap(n.all);
    if(melee_animation_)melee_animation_.swap(n.melee);
    authority_calls_.fault=false;p.impl_.reset();return true;
}
}
