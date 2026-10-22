#include "helpers/gameplay_fixture.hpp"
#include "halo2/simulation_authority.hpp"
#include "halo2/math.hpp"
#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/native_unit_interaction.hpp"
#include "source_device_snapshot_check.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <tuple>
#include <atomic>
#include <cstdlib>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid/active simulation continuation accepted");}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
struct Program {
    ScriptProgram data;
    std::uint32_t node(ScriptNode n){const auto i=std::uint32_t(data.nodes.size());n.salt=std::uint16_t(0xe000+i);data.nodes.push_back(n);return(std::uint32_t(n.salt)<<16)|i;}
    std::uint32_t literal(std::uint32_t bits,unsigned type=8){return node({0,std::uint16_t(type),std::uint16_t(type),9,null_datum,0,bits});}
    std::uint32_t real(float v){return literal(std::bit_cast<std::uint32_t>(v),6);}
    std::uint32_t text(std::string value){const auto at=std::uint32_t(data.strings.size());data.strings+=value;data.strings+='\0';return node({0,9,9,9,null_datum,at,0});}
    std::uint32_t global(){return node({0,8,8,13,null_datum,0,0});}
    std::uint32_t call(std::string name,std::vector<std::uint32_t> args={},unsigned type=4){
        const auto at=std::uint32_t(data.strings.size());data.strings+=name;data.strings+='\0';
        for(unsigned i=0;i<args.size();++i)data.nodes[args[i]&65535u].next=i+1<args.size()?args[i+1]:null_datum;
        const auto fn=node({0,0,2,9,args.empty()?null_datum:args[0],at,0});return node({0,0,std::uint16_t(type),8,null_datum,at,fn});
    }
};
void weapon(const WeaponState &a,const WeaponState &b){
    check(std::tie(a.definition,a.loaded,a.reserve,a.burst_left,a.last_shot,a.reload_started,a.reload_empty,a.trigger_armed,
        a.source_datum,a.ready_countdown,a.ready_clock,a.overheat_handle,a.energy_blocking_ticks,a.firing_effect_index,
        a.firing_effect_remaining,a.firing_effect_visited,a.charge_fraction,a.source_runtime_published)==
        std::tie(b.definition,b.loaded,b.reserve,b.burst_left,b.last_shot,b.reload_started,b.reload_empty,b.trigger_armed,
        b.source_datum,b.ready_countdown,b.ready_clock,b.overheat_handle,b.energy_blocking_ticks,b.firing_effect_index,
        b.firing_effect_remaining,b.firing_effect_visited,b.charge_fraction,b.source_runtime_published)&&
        same(a.delay,b.delay)&&same(a.reload,b.reload)&&same(a.cooling_inhibitor,b.cooling_inhibitor)&&
        a.shader.item_flags==b.shader.item_flags&&a.shader.weapon_flags==b.shader.weapon_flags&&
        same(a.shader.heat,b.shader.heat)&&same(a.shader.power_fraction,b.shader.power_fraction),"Unit weapon continuation differs");
}
void unit(const Unit &a,const Unit &b){
    check(a.seat_action.phase==b.seat_action.phase&&a.seat_action.started==b.seat_action.started&&
        same(a.seat_action.entry_position,b.seat_action.entry_position),"Unit seat transition continuation differs");
    check(std::tie(a.source_datum,a.source_player,a.source_previous_player,a.source_membership,a.source_death_tick,
        a.source_forward_9,a.source_forward_other,a.source_parent,a.source_action_word,a.source_seat,a.exists,a.visible,a.bsp_hidden,
        a.bsp,a.controlled,a.team,a.state,a.zoom_reset_serial,a.custom_animation_tick,a.custom_animation_loop)==
        std::tie(b.source_datum,b.source_player,b.source_previous_player,b.source_membership,b.source_death_tick,
        b.source_forward_9,b.source_forward_other,b.source_parent,b.source_action_word,b.source_seat,b.exists,b.visible,b.bsp_hidden,
        b.bsp,b.controlled,b.team,b.state,b.zoom_reset_serial,b.custom_animation_tick,b.custom_animation_loop)&&
        same(a.body.position,b.body.position)&&same(a.body.velocity,b.body.velocity)&&same(a.body.support_normal,b.body.support_normal)&&
        a.body.grounded==b.body.grounded&&a.body.support_triangle==b.body.support_triangle&&
        same(a.vitality.body,b.vitality.body)&&same(a.vitality.shield,b.vitality.shield)&&
        std::tie(a.vitality.body_stun,a.vitality.shield_stun,a.vitality.dead,a.vitality.paused,a.vitality.body_recharging,a.vitality.shield_recharging)==
        std::tie(b.vitality.body_stun,b.vitality.shield_stun,b.vitality.dead,b.vitality.paused,b.vitality.body_recharging,b.vitality.shield_recharging)&&
        same(a.definition.vitality.health,b.definition.vitality.health)&&same(a.definition.vitality.shield,b.definition.vitality.shield)&&
        same(a.pitch,b.pitch)&&same(a.yaw,b.yaw)&&same(a.crouch,b.crouch),"Unit identity/body/vitality/animation continuation differs");
    weapon(a.weapon,b.weapon);
    const auto &x=a.source_inventory,&y=b.source_inventory;
    check(x.slots==y.slots&&x.current==y.current&&x.requested==y.requested&&x.ready_ticks==y.ready_ticks&&
        x.inventory_sequence==y.inventory_sequence&&x.replicated_sequence==y.replicated_sequence,"Full inventory identity/sequence differs");
    for(unsigned i=0;i<4;++i){check(bool(x.inactive_weapons[i])==bool(y.inactive_weapons[i]),"Inactive ammo owner lost");
        if(x.inactive_weapons[i])weapon(*x.inactive_weapons[i],*y.inactive_weapons[i]);}
    for(unsigned i=0;i<4;++i){const auto &p=a.source_damage_history[i],&q=b.source_damage_history[i];
        check(std::tie(p.tick,p.object,p.player,p.cause)==std::tie(q.tick,q.object,q.player,q.cause)&&same(p.amount,q.amount),"Captured unit damage history differs");}
}
void root(Simulation &a,Simulation &b,Simulation::AuthorityDigest binding){
    unit(a.player(),b.player());check(a.actors().size()==b.actors().size(),"Actor bank shape differs");
    for(unsigned i=0;i<a.actors().size();++i)unit(a.actors()[i],b.actors()[i]);
    const auto x=a.authority_checkpoint(binding),y=b.authority_checkpoint(binding);
    check(x.vehicle_rows.size()==y.vehicle_rows.size()&&x.vehicle_flips.size()==y.vehicle_flips.size()&&
        x.vehicle_engine==y.vehicle_engine&&x.vehicle_timer_option==y.vehicle_timer_option,
        "Vehicle row/flip bank configuration differs");
    for(unsigned i=0;i<x.vehicle_rows.size();++i){const auto &p=x.vehicle_rows[i],&q=y.vehicle_rows[i];
        check(std::tie(p.object,p.timer,p.empty_counter)==std::tie(q.object,q.timer,q.empty_counter),
            "Vehicle respawn row continuation differs");}
    for(unsigned i=0;i<x.vehicle_flips.size();++i){const auto &p=x.vehicle_flips[i],&q=y.vehicle_flips[i];
        check(std::tie(p.object,p.direction,p.ticks)==std::tie(q.object,q.direction,q.ticks)&&same(p.velocity,q.velocity),
            "Vehicle flip continuation differs");}
    check(x.ticks==y.ticks&&x.shots==y.shots&&x.hits==y.hits&&x.random_state==y.random_state&&x.scripts==y.scripts&&
        x.bsp_binding==y.bsp_binding&&x.unit_interaction_binding==y.unit_interaction_binding&&
        x.bsp_revision==y.bsp_revision&&x.bsp_collision.order==y.bsp_collision.order&&
        x.all_collision->order==y.all_collision->order&&x.projectiles.size()==y.projectiles.size(),"Root clock/RNG/script/collision/projectile frontier differs");
    test::same_device_snapshot(x.devices.state,y.devices.state);test::same_device_events(x.device_events,y.device_events);
    check(std::ranges::equal(a.object_lifetimes().events(),b.object_lifetimes().events())&&
        std::ranges::equal(a.object_lifetimes().pool().elements(),b.object_lifetimes().pool().elements())&&
        std::ranges::equal(a.source_object_bodies().arena(),b.source_object_bodies().arena())&&
        std::ranges::equal(a.source_object_bodies().scratch(),b.source_object_bodies().scratch())&&
        std::ranges::equal(a.source_object_bodies().handles(),b.source_object_bodies().handles()),"Root allocator/storage continuation differs");
    for(unsigned i=0;i<x.projectiles.size();++i){const auto &p=x.projectiles[i],&q=y.projectiles[i];
        check(p.source_datum==q.source_datum&&p.weapon==q.weapon&&p.owner==q.owner&&p.bsp==q.bsp&&p.barrel_index==q.barrel_index&&
            std::tie(p.captured_owner.player,p.captured_owner.unit,p.captured_owner.designator)==
            std::tie(q.captured_owner.player,q.captured_owner.unit,q.captured_owner.designator)&&
            same(p.position,q.position)&&same(p.velocity,q.velocity)&&same(p.traveled,q.traveled),"Native weapon projectile continuation differs");}
    const auto &ac=a.sound_classes().state(),&bc=b.sound_classes().state();
    for(unsigned i=0;i<ac.size();++i)check(same(ac[i].current,bc[i].current)&&same(ac[i].remaining,bc[i].remaining)&&same(ac[i].target,bc[i].target),"Audio wall-clock envelope differs");
    check(same(x.weather.current,y.weather.current)&&same(x.weather.target,y.weather.target)&&same(x.weather.rate,y.weather.rate)&&
        same(x.weather_clock.previous,y.weather_clock.previous)&&same(x.weather_clock.elapsed,y.weather_clock.elapsed)&&
        x.training.actions==y.training.actions&&x.presentation.transitions[0].elapsed==y.presentation.transitions[0].elapsed,
        "Independent view/audio/training/presentation clocks differ");
}
void owned(const char *xbe,const char *map){
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    auto acontent=ac.load_content(0,true),bcontent=bc.load_content(0,true);
    Program p;p.data.globals.push_back({"counter",8,p.literal(0)});
    const auto sequence=p.call("begin",{
        p.call("custom_animation_loop",{p.literal(0),p.literal(1),p.literal(2),p.literal(0)}),
        p.call("unit_set_maximum_vitality",{p.literal(0),p.real(100),p.real(200)}),
        p.call("weather_change_intensity",{p.real(4),p.real(.35f)}),
        p.call("sound_class_set_gain",{p.text("music"),p.real(.5f),p.literal(90,7)}),
        p.call("fade_out",{p.real(.1f),p.real(.2f),p.real(.4f),p.literal(60,7)}),p.call("sleep",{p.literal(5)}),
        p.call("begin_random",{p.call("begin",{p.call("sleep",{p.literal(9)}),p.call("set",{p.global(),p.literal(11)},8)}),
                              p.call("begin",{p.call("sleep",{p.literal(7)}),p.call("set",{p.global(),p.literal(22)},8)})},3),
        p.call("sleep",{p.literal(25)}),p.call("set",{p.global(),p.literal(99)},8)});
    p.data.scripts.push_back({"main",0,4,sequence});acontent.scripts=p.data;bcontent.scripts=p.data;
    // Explicit native program/custom-clip/admission inputs, not authored map
    // scripts, Xbox custom-animation acceptance or an authenticated peer seal.
    AnimationClip clip;clip.source_graph=1;clip.source_index=2;clip.name="native root ownership fixture";clip.frame_count=3;
    Simulation *ap=nullptr,*bp=nullptr;bool interrupt=false;unsigned a_loads=0,b_loads=0;
    const Simulation::AuthorityDigest binding{std::byte{0x42},std::byte{0x15}};
    Simulation a(acontent,0,ac.spawns().front(),[&](auto,auto){++a_loads;rejects([&]{ap->authority_checkpoint(binding);});if(interrupt)throw 17;return clip;},
        {},{},SourceSceneGameContext{2,2});
    Simulation b(bcontent,0,bc.spawns().front(),[&](auto,auto){++b_loads;rejects([&]{bp->authority_checkpoint(binding);});return clip;},
        {},{},SourceSceneGameContext{2,2});ap=&a;bp=&b;
    a.equip(ac.slayer_starting_weapon(5));
    const auto storage=a.allocate_source_body_storage(65);a.source_body_storage(storage)[20]=std::byte{0x57};
    auto seed=a.authority_checkpoint(binding);auto &life=*seed.owned_lifetimes;auto &u=seed.player.value;
    const auto active=life.reserve(2);life.publish_native(active,u.weapon.definition);u.weapon.source_datum=active;
    u.source_inventory.slots[0]=active;u.source_inventory.current[0]=u.source_inventory.requested[0]=0;
    const auto other=std::ranges::find_if(acontent.weapons,[&](const auto &w){return w.first!=u.weapon.definition&&w.second.capacity>0;});
    check(other!=acontent.weapons.end(),"Actual secondary weapon unavailable");const auto secondary=life.reserve(2);life.publish_native(secondary,other->first);
    u.source_inventory.slots[1]=secondary;u.source_inventory.inactive_weapons[1]=WeaponState{other->first,3,21};
    u.source_inventory.inactive_weapons[1]->source_datum=secondary;u.source_inventory.ready_ticks[1]=3;u.source_inventory.inventory_sequence=19;
    record_damage_history(u.source_damage_history,0,.25f,0xd1230001,0xd1230002,3);
    auto projectile=std::ranges::find_if(acontent.weapons,[](const auto &w){return !w.second.instantaneous&&w.second.projectile_speed>0&&w.second.range>10&&!w.second.needler_projectile;});
    check(projectile!=acontent.weapons.end(),"Actual ordinary projectile metadata unavailable");
    const auto ph=life.reserve(5);life.publish_native(ph,projectile->second.projectile);
    float top=u.body.position.z;for(const auto &t:a.collision().triangles())top=std::max({top,t.a.z,t.b.z,t.c.z});
    Simulation::Projectile flight;flight.position={u.body.position.x,u.body.position.y,top+100};flight.velocity={1,0,10};flight.forward={1,0,0};flight.up={0,0,1};
    flight.weapon=projectile->first;flight.owner=0;flight.bsp=0;flight.source_datum=ph;
    flight.captured_owner={0xd4560001,0xd4560002,3};seed.projectiles.push_back(flight);
    auto staged=a.prepare_authority(seed,binding);check(a.commit_authority(std::move(staged)),"Declared native caller state admission failed");
    a.run_script("main");
    for(unsigned i=0;i<7;++i){a.step({});a.advance_audio(.01f);a.advance_weather_view(500+double(i)/30,0);}
    const auto saved=a.authority_checkpoint(binding),before=b.authority_checkpoint(binding);
    check(saved.player.animation&&!saved.player.value.custom_animation&&a_loads==1&&!saved.projectiles.empty()&&
        saved.player.value.definition.vitality.health==100&&saved.player.value.definition.vitality.shield==200,
        "Root fixture omitted custom animation/dynamic maxima/in-flight projectile");
    bool random=false;for(const auto &t:saved.scripts.threads)for(const auto &f:t.stack)random|=f.name=="begin_random"&&f.random_visited;
    check(random,"Root fixture did not capture a partially visited random expression");
    {auto prep=b.prepare_authority(saved,binding);rejects([&]{b.step({});});rejects([&]{b.advance_audio(.1f);});
        rejects([&]{b.source_primary_random();});rejects([&]{b.take_sounds();});rejects([&]{b.request_bsp(0);});
        rejects([&]{b.authority_checkpoint(binding);});check(!a.commit_authority(std::move(prep)),"Foreign simulation token committed");}
    check(b.authority_checkpoint(binding).ticks==before.ticks,"Cancelled root changed current tick");
    auto bad=saved;++bad.version;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.service_roles^=1;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.admitted_binding[0]^=std::byte{1};rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.custom_animation=a.player().custom_animation;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.body.support_triangle=SIZE_MAX;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.definition.radius+=.25f;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.definition.vitality.health=std::numeric_limits<float>::quiet_NaN();rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.seat_action.entry_position.x=std::numeric_limits<float>::quiet_NaN();rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.seat_action.phase=Unit::SeatAction::Entering;
    bad.player.value.seat_action.started=saved.ticks+1;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.unit_interaction_binding[0]^=std::byte{1};rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.player.value.source_inventory.slots[0]^=0x10000u;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.object_bodies=SourceObjectBodyStorage(512);rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.bsp_collision.order[0]=bad.bsp_collision.order[1];rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.bsp_collision.nodes[0].left=0;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.scripts.version=2;rejects([&]{b.prepare_authority(bad,binding);});
    bad=saved;bad.projectiles[0].barrel_index=INT16_MAX;rejects([&]{b.prepare_authority(bad,binding);});
    const auto interactions=bcontent.unit_interactions;
    check(bool(interactions)&&!interactions->netgame_placements.empty(),"Actual Zanzibar vehicle bindings unavailable");
    const auto mismatch=[&](auto edit){
        auto changed=std::make_shared<NativeUnitInteractionAssets>(*interactions);edit(*changed);
        bcontent.unit_interactions=changed;rejects([&]{b.prepare_authority(saved,binding);});
        bcontent.unit_interactions=interactions;
    };
    mismatch([](auto &assets){assets.flipped_up_threshold+=.01f;});
    mismatch([](auto &assets){++assets.netgame_placements.begin()->second;});
    mismatch([](auto &assets){
        auto vehicle=std::ranges::find_if(assets.units,[](const auto &entry){return !entry.second.seats.empty();});
        check(vehicle!=assets.units.end(),"Actual vehicle seat unavailable");vehicle->second.seats.front().radius+=.01f;
    });
    mismatch([](auto &assets){
        auto actor=std::ranges::find_if(assets.units,[](const auto &entry){return !entry.second.seat_animations.empty();});
        check(actor!=assets.units.end(),"Actual seat animations unavailable");
        auto animation=std::ranges::find_if(actor->second.seat_animations,[](const auto &entry){
            return !entry.second.entries.empty()&&bool(entry.second.entries.front().clip);});
        check(animation!=actor->second.seat_animations.end(),"Actual seat entry clip unavailable");
        auto clip=std::make_shared<AnimationClip>(*animation->second.entries.front().clip);++clip->frame_count;
        animation->second.entries.front().clip=clip;
    });
    bcontent.unit_interactions.reset();rejects([&]{b.prepare_authority(saved,binding);});bcontent.unit_interactions=interactions;
    const auto row=interactions->netgame_placements.begin()->first;
    const auto respawn_seconds=bcontent.netgame_equipment.at(row).respawn_seconds;
    ++bcontent.netgame_equipment.at(row).respawn_seconds;rejects([&]{b.prepare_authority(saved,binding);});
    bcontent.netgame_equipment.at(row).respawn_seconds=respawn_seconds;
    check(b.authority_checkpoint(binding).ticks==before.ticks,"Rejected root left a changed/staged owner");
    auto previous_contact=saved;previous_contact.player.value.exists=false;
    previous_contact.player.value.body.support_triangle=previous_contact.all_collision->triangles.size()+1;
    staged=b.prepare_authority(previous_contact,binding);check(b.commit_authority(std::move(staged)),"Retained inactive-body contact was rejected");
    check(b.authority_checkpoint(binding).player.value.body.support_triangle==previous_contact.player.value.body.support_triangle,
        "Previous-view inactive contact was rewritten during import/export");
    staged=b.prepare_authority(saved,binding);check(b.commit_authority(std::move(staged))&&!b.commit_authority(std::move(staged)),"Whole native simulation publication failed/reused token");
    check(b_loads==0&&a.player().custom_animation.get()!=b.player().custom_animation.get(),"Imported unit clip retained a source allocation/invoked target loader");root(a,b,binding);
    for(unsigned i=0;i<90;++i){Controls controls;controls.forward=i%7<3?.25f:0;controls.yaw=.1f;controls.crouch=i%11<2;
        a.step(controls);b.step(controls);a.advance_audio(.01f);b.advance_audio(.01f);
        a.advance_weather_view(500+double(i+7)/30,0);b.advance_weather_view(500+double(i+7)/30,0);root(a,b,binding);}
    check(a.scripts().globals()[0].number()==99,"Imported native random/sleep expression did not finish");
    check(a.append_source_body_storage(storage,0,23,2)==b.append_source_body_storage(storage,0,23,2),"Restored raw storage did not use its local allocator");
    a.release_source_body_storage(storage);b.release_source_body_storage(storage);root(a,b,binding);
    const auto recover=a.authority_checkpoint(binding);a.run_script("main");interrupt=true;bool thrown=false;
    try{a.step({});}catch(int v){thrown=v==17;}check(thrown,"Declared outer loader interruption not reached");
    rejects([&]{a.authority_checkpoint(binding);});staged=a.prepare_authority(recover,binding);
    check(a.commit_authority(std::move(staged)),"Faulted simulation did not recover admitted whole state");
    a.request_checkpoint();rejects([&]{a.authority_checkpoint(binding);});
    std::cout<<"PASS actual Zanzibar/TU1.5 native simulation metadata with declared program/clip/projectile inputs: ninety future unit/inventory/projectile/RNG/script/view/audio/collision steps, local ownership, staged admission and interrupted-root recovery\n";
}
void vehicle(const char *xbe,const char *map){
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    const auto ca=ac.load_content(0,true),cb=bc.load_content(0,true);
    Simulation a(ca,0,ac.spawns().front()),b(cb,0,bc.spawns().front());
    const Simulation::AuthorityDigest binding{std::byte{0x56},std::byte{0x15}};
    // Standalone Simulation leaves its player identity to its caller. Supply
    // a native-owned biped generation, as required by flip target admission.
    auto native_player=a.authority_checkpoint(binding);
    auto &life=*native_player.owned_lifetimes;
    const auto player_handle=life.reserve(0);life.publish_native(player_handle,ca.player.datum);
    native_player.player.value.source_datum=player_handle;
    auto player_owner=a.prepare_authority(native_player,binding);
    check(a.commit_authority(std::move(player_owner)),"Declared native player identity was rejected");
    for(unsigned tick=0;tick<30;++tick)a.step({});
    // Declared native test positions search actual authored vehicles through
    // the production prompt/admission path. This is not a retail spawn, an
    // original TU1.5 animation controller or original vehicle dynamics test.
    std::vector<std::uint32_t> vehicles;
    for(const auto &o:a.scene().objects())if(o.handle!=null_datum&&ca.objects[o.placement].type==1&&o.visible)
        vehicles.push_back(o.handle);
    NativeActionPrompt entry;Spawn approach;bool admitted=false;
    for(const auto handle:vehicles){
        const auto position=a.scene().get(handle).position;
        for(float radius:{.2f,.5f,.8f,1.1f,1.5f,2.f,2.5f}){
            for(unsigned angle=0;angle<32;++angle){
                const float theta=float(angle)*6.2831853f/32;
                Spawn at;at.position=position+direction(theta)*radius;at.position.z=position.z;
                at.facing=theta+3.14159265f;at.bsp=0;
                const auto floor=a.collision().raycast(at.position+Vec3{0,0,2},{0,0,-1},4);
                if(!floor)continue;at.position.z=floor->position.z+.01f;test::place_player(a,at);
                const auto prompt=a.unit_action_prompt(a.player());
                if(prompt.target.object!=handle||(prompt.target.action!=5&&prompt.target.action!=6))continue;
                check(a.take_unit_action(const_cast<Unit&>(a.player()),prompt),"Production displayed vehicle entry was rejected");
                entry=prompt;approach=at;admitted=true;break;
            }
            if(admitted)break;
        }
        if(admitted)break;
    }
    check(admitted&&a.player().seat_action.phase==Unit::SeatAction::Entering&&bool(a.player().custom_animation),
        "No actual vehicle entry transition captured");
    const auto restore=[&]{
        const auto saved=a.authority_checkpoint(binding);
        auto staged=b.prepare_authority(saved,binding);
        check(b.commit_authority(std::move(staged)),"Native vehicle transition import failed");
        if(a.player().custom_animation)check(a.player().custom_animation.get()!=b.player().custom_animation.get(),
            "Restored seat transition retained another owner's clip");
        root(a,b,binding);
    };
    const auto advance=[&]{Controls controls;controls.yaw=approach.facing;a.step(controls);b.step(controls);root(a,b,binding);};
    restore();unsigned entering_ticks=0;
    while(a.player().seat_action.phase!=Unit::SeatAction::Seated&&entering_ticks++<120)advance();
    check(entering_ticks>0&&entering_ticks<=120&&a.player().source_parent==entry.target.object&&
        a.player().source_seat==entry.target.index,"Restored native seat entry did not finish");
    check(a.take_unit_action(const_cast<Unit&>(a.player()),{{0,entry.target.index,entry.target.object},entry.definition,0})&&
        a.player().seat_action.phase==Unit::SeatAction::Exiting,"Production vehicle exit did not begin");
    restore();unsigned exit_ticks=0;
    while(a.player().source_parent!=null_datum&&exit_ticks++<120)advance();
    check(exit_ticks>0&&exit_ticks<=120&&a.player().seat_action.phase==Unit::SeatAction::None,
        "Restored native seat exit did not finish");
    // An explicitly declared overturned native scene pose exercises the flip
    // continuation owner; it supplies no original Havok result or Xbox pose.
    auto declared_pose=a.authority_checkpoint(binding);
    auto &object=declared_pose.scene.objects.at(declared_pose.scene.slots.at(entry.target.object));
    Transform overturned;
    if(object.solved_transform)overturned=*object.solved_transform;
    else {const auto basis=authored_basis(object.rotation);overturned.position=object.position;
        overturned.scale=object.scale;overturned.basis={basis.forward,basis.left,basis.up};}
    overturned.basis[1]=overturned.basis[1]*-1.f;overturned.basis[2]=overturned.basis[2]*-1.f;
    object.solved_transform=overturned;object.solved_nodes.reset();
    auto declared=a.prepare_authority(declared_pose,binding);
    check(a.commit_authority(std::move(declared)),"Declared native overturned scene pose was rejected");
    test::place_player(a,approach);
    const auto flip=a.unit_action_prompt(a.player());
    check(flip.target.object==entry.target.object&&flip.target.action==8&&
        a.take_unit_action(const_cast<Unit&>(a.player()),flip),"Production overturned-vehicle flip did not begin");
    check(a.authority_checkpoint(binding).vehicle_flips.size()==1,"Active native flip was not captured");
    restore();
    for(unsigned tick=0;tick<65;++tick){
        advance();const auto &p=a.scene().get(entry.target.object),&q=b.scene().get(entry.target.object);
        check(bool(p.solved_transform)==bool(q.solved_transform),"Restored native flip lost its pose");
        if(p.solved_transform){
            check(same(p.solved_transform->position,q.solved_transform->position)&&
                same(p.solved_transform->scale,q.solved_transform->scale),"Restored native flip root differs");
            for(unsigned axis=0;axis<3;++axis)check(same(p.solved_transform->basis[axis],q.solved_transform->basis[axis]),
                "Restored native flip rotation differs");
        }
    }
    check(a.authority_checkpoint(binding).vehicle_flips.empty(),"Restored native flip did not retire");
    std::cout<<"PASS actual Zanzibar/TU1.5 vehicle definitions with declared native approach/overturned poses: production seat entry/exit/flip, deep clip ownership and continued attachment/animation/respawn/flip clocks\n";
}
struct Directory {
    std::filesystem::path path;
    Directory(){auto value=(std::filesystem::temp_directory_path()/"halo2-simulation-continuation-XXXXXX").string();
        check(::mkdtemp(value.data()),"Cannot create private native world files");path=value;}
    ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
};
struct Clock final:PhysicsLiveClockSource {
    std::uint32_t time{};mutable std::atomic<std::uint64_t> counter{};
    std::uint32_t milliseconds()const noexcept override{return time;}
    std::uint64_t cycles()const noexcept override{return counter.fetch_add(1000);}
    std::uint64_t utc_filetime()const noexcept override{return 132000000000000000ull;}
};
void source(const char *xbe,const char *map){
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    const auto authored_a=ac.load_content(0,true),authored_b=bc.load_content(0,true);
    const auto content=[](const LevelContent &authored,Spawn spawn){
        LevelContent c;c.bsp_count=authored.bsp_count;c.player=authored.player;c.movement=authored.movement;
        c.original_data=authored.original_data;c.collision=authored.collision;c.navigation=authored.navigation;
        c.multiplayer_spawn=authored.multiplayer_spawn;
        // Explicit native caller placement backed by actual TU1.5 construction,
        // not an authored Lockout biped row or a full multiplayer match fixture.
        ScenarioObject p;p.type=0;p.name=0;p.definition=authored.player.datum;p.position=spawn.position;
        c.objects.push_back(p);c.object_names.push_back({"native composed biped",0,0});
        c.devices.resize(1);c.prop_sandbox.resize(1);c.static_scenery_collision.resize(1);return c;
    };
    auto ca=content(authored_a,ac.spawns().front()),cb=content(authored_b,bc.spawns().front());
    Directory af,bf;auto at=std::make_shared<Clock>(),bt=std::make_shared<Clock>();at->time=1000;bt->time=900000;
    MultiplayerObjectRuntime wa(xbe,ac,0,af.path,at),wb(xbe,bc,0,bf.path,bt);
    auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
    wa.configure_session(variant,1,30,0x12345678);wb.configure_session(variant,1,30,0x12345678);
    Simulation a(ca,0,ac.spawns().front(),{},{},{},SourceSceneGameContext{2,2},
        NativeObjectLifetimes(wa.memory(),wa.object_pool()),{[&](const auto &p,SourceScenarioBirth){return wa.create_biped(p.definition,p.position,{1,0,0});},[&](auto h){wa.destroy_object(h);}});
    Simulation b(cb,0,bc.spawns().front(),{},{},{},SourceSceneGameContext{2,2},
        NativeObjectLifetimes(wb.memory(),wb.object_pool()),{[&](const auto &p,SourceScenarioBirth){return wb.create_biped(p.definition,p.position,{1,0,0});},[&](auto h){wb.destroy_object(h);}});
    const Simulation::AuthorityDigest binding{std::byte{0x42},std::byte{0x15}};
    check(a.scene().resolve(0)!=null_datum,"Actual composed TU1.5 scene biped unavailable");
    rejects([&]{a.authority_checkpoint(binding);});const auto saved=a.authority_checkpoint(binding,true);
    auto &am=wa.memory(),&bm=wb.memory();const auto before=bm.checkpoint();
    {
        auto world=wb.prepare_authority(wa.authority_checkpoint());
        auto sim=b.prepare_authority(saved,binding,&world.replacement_memory());
        check(!b.commit_authority(std::move(sim))&&bm.checkpoint()==before,"Simulation published a source view before its bank");
    }
    check(bm.checkpoint()==before,"Cancelled coupled source transaction changed backing");
    auto world=wb.prepare_authority(wa.authority_checkpoint());
    auto sim=b.prepare_authority(saved,binding,&world.replacement_memory());
    check(wb.commit_authority(std::move(world))&&b.commit_authority(std::move(sim)),"Coupled TU1.5 world/simulation publication failed");
    check(am.checkpoint()==bm.checkpoint()&&a.object_lifetimes().source_authority_checkpoint()==b.object_lifetimes().source_authority_checkpoint(),
        "Coupled source authority changed actual bytes or lifetime history");
    for(unsigned tick=0;tick<20;++tick){at->time+=33;bt->time+=33;wa.step_objects();wb.step_objects();a.step({});b.step({});
        check(am.checkpoint()==bm.checkpoint(),"Future source world ticks diverged after whole simulation composition");
        const auto x=a.authority_checkpoint(binding,true),y=b.authority_checkpoint(binding,true);
        check(x.ticks==y.ticks&&x.scripts==y.scripts&&x.source_lifetimes==y.source_lifetimes&&x.scene.names==y.scene.names,
            "Future coupled simulation/source identity clocks diverged");}
    std::cout<<"PASS actual TU1.5 world/pool/simulation staged composition with explicit native biped placement, premature/cancelled publication and twenty subsequent complete source object ticks\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;check(argc==4,"Supply TU1.5 XBE, Zanzibar and Lockout MAPs");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2])||!std::filesystem::exists(argv[3]))return 77;
    owned(argv[1],argv[2]);vehicle(argv[1],argv[2]);source(argv[1],argv[3]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
