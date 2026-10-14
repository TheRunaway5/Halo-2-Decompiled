#include "halo2/native_grenade.hpp"
#include "halo2/simulation.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <tuple>

using namespace halo2;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid/active grenade continuation accepted");}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
bool same(const Transform &a,const Transform &b){if(!same(a.scale,b.scale)||!same(a.position,b.position))return false;
    for(unsigned i=0;i<3;++i)if(!same(a.basis[i],b.basis[i]))return false;return true;}
void same(const NativeGrenadeProjectile &a,const NativeGrenadeProjectile &b){
    check(std::tie(a.datum,a.definition,a.parent,a.player,a.unit,a.designator,a.type,a.native_owner,a.flags,a.state,a.age)==
        std::tie(b.datum,b.definition,b.parent,b.player,b.unit,b.designator,b.type,b.native_owner,b.flags,b.state,b.age) &&
        same(a.position,b.position)&&same(a.velocity,b.velocity)&&same(a.forward,b.forward)&&same(a.up,b.up)&&
        same(a.timer,b.timer)&&same(a.timer_step,b.timer_step)&&same(a.arming,b.arming)&&same(a.arming_step,b.arming_step)&&same(a.traveled,b.traveled),
        "Restored grenade projectile state differs");
}
void same(const NativeGrenadeEvent &a,const NativeGrenadeEvent &b){check(
    std::tie(a.kind,a.unit,a.projectile,a.definition,a.effect,a.activity,a.graph,a.clip,a.counter,a.phase)==
    std::tie(b.kind,b.unit,b.projectile,b.definition,b.effect,b.activity,b.graph,b.clip,b.counter,b.phase) &&
    same(a.position,b.position)&&same(a.velocity,b.velocity),"Restored queued grenade event differs");}
void same(const NativeGrenadeDetonation &a,const NativeGrenadeDetonation &b){
    check(std::tie(a.serial,a.tick,a.projectile,a.definition,a.effect,a.player,a.unit,a.colors.a,a.colors.b)==
        std::tie(b.serial,b.tick,b.projectile,b.definition,b.effect,b.player,b.unit,b.colors.a,b.colors.b) &&
        same(a.position,b.position)&&same(a.forward,b.forward)&&same(a.up,b.up)&&same(a.controls.a,b.controls.a)&&same(a.controls.b,b.controls.b)&&
        a.source_locations.size()==b.source_locations.size(),"Restored grenade detonation/journal differs");
    for(unsigned i=0;i<a.source_locations.size();++i)check(a.source_locations[i].name==b.source_locations[i].name &&
        same(a.source_locations[i].frame,b.source_locations[i].frame),"Restored completed source effect location differs");
}
void same(const NativeGrenadeSandbox::AuthorityCheckpoint &a,const NativeGrenadeSandbox::AuthorityCheckpoint &b){
    check(a.version==b.version && a.binding==b.binding && a.initial_unit==b.initial_unit && a.source_driven==b.source_driven &&
        a.unit_bindings==b.unit_bindings && a.weapon_clip_bindings==b.weapon_clip_bindings && a.selected_clip_bindings==b.selected_clip_bindings &&
        a.detonation_serial==b.detonation_serial && a.projectiles.size()==b.projectiles.size() && a.pending_events.size()==b.pending_events.size() &&
        a.detonations.size()==b.detonations.size() && a.attachments.size()==b.attachments.size(),"Grenade continuation shape/bindings/frontier differ");
    for(unsigned i=0;i<a.projectiles.size();++i)same(a.projectiles[i],b.projectiles[i]);
    for(unsigned i=0;i<a.pending_events.size();++i)same(a.pending_events[i],b.pending_events[i]);
    for(unsigned i=0;i<a.detonations.size();++i)same(a.detonations[i],b.detonations[i]);
    for(const auto &[h,x]:a.attachments){const auto &y=b.attachments.at(h);check(x.first==y.first && same(x.second.position,y.second.position)&&
        same(x.second.forward,y.second.forward)&&same(x.second.up,y.second.up),"Grenade local attachment frame differs");}
}
void same(const Unit &a,const Unit &b){check(a.grenades==b.grenades && a.grenade_action.source==b.grenade_action.source &&
    std::tie(a.grenade_action.graph,a.grenade_action.clip,a.grenade_action.activity,a.grenade_action.animation_active,a.source_action_word,a.zoom_reset_serial)==
    std::tie(b.grenade_action.graph,b.grenade_action.clip,b.grenade_action.activity,b.grenade_action.animation_active,b.source_action_word,b.zoom_reset_serial) &&
    same(a.grenade_action.frame,b.grenade_action.frame),"Restored unit grenade action/debit differs");}
std::shared_ptr<const Cache> cache(const char *map,const char *xbe){return std::make_shared<const Cache>(map,std::filesystem::path{},OriginalData::load_xbe(xbe));}
void native(const char *map,const char *xbe,int type) {
    auto ac=cache(map,xbe),bc=cache(map,xbe);const auto c=ac->load_content(0,true);
    const auto constants=*ac->original_data()->grenade_constants;
    NativeGrenadeSandbox a(ac,c.player.datum,constants),b(bc,c.player.datum,constants);
    NativeObjectLifetimes al,bl;std::array<Unit,1> au,bu;
    auto &u=au[0];u.definition=c.player;u.source_datum=al.reserve(0);al.publish_native(u.source_datum,c.player.datum);
    u.source_player=0xd1230000u;u.source_membership=2;u.weapon.definition=ac->slayer_starting_weapon(5);
    u.body.position=ac->spawns().front().position;u.body.position.z+=2;u.pitch=-.15f;
    u.grenades={2,2};u.grenade_current=u.grenade_requested=std::int8_t(type);bu=au;bl=al;
    const auto allocate=[&](std::uint8_t kind){rejects([&]{a.authority_checkpoint(au);});return al.reserve(kind);};
    const auto constructed=[&](auto h,auto d){al.publish_native(h,d);rejects([&]{a.authority_checkpoint(au);});};
    unsigned tick=0;
    for(;tick<20;++tick){a.update_unit(u,tick==0,0,allocate,{},constructed);
        if(!a.projectiles().empty() && a.projectiles()[0].parent!=null_datum)break;}
    check(tick<20 && u.grenades[unsigned(type)]==1,"Actual native preparation did not produce/debit a held grenade");
    const auto &w=c.weapons.at(u.weapon.definition);
    const auto fp=ac->load_unit_weapon_action(w.first_person_animation,0x030000d9,0x030000d9,0x030000d9,0x0d000021);
    check(a.action_clip(fp.source_graph,fp.source_index),"Actual selected clip residency unavailable");
    const auto saved=a.authority_checkpoint(au),before=b.authority_checkpoint(bu);
    check(!saved.attachments.empty() && !saved.weapon_clip_bindings.empty() && !saved.selected_clip_bindings.empty() && !saved.pending_events.empty(),
        "Grenade fixture omitted attachment, decoded caches or pending events");
    {auto staged=b.prepare_authority(saved,au);rejects([&]{b.take_events();});rejects([&]{b.cancel_unit(bu[0]);});
        rejects([&]{b.action_clip(fp.source_graph,fp.source_index);});check(!a.commit_authority(std::move(staged)),"Foreign grenade owner committed");}
    same(before,b.authority_checkpoint(bu));
    auto bad=saved;bad.version=2;rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.attachments.clear();rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.attachments.begin()->second.first=UINT_MAX;rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.projectiles[0].definition^=1;rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.projectiles[0].timer=std::numeric_limits<float>::infinity();rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.weapon_clip_bindings.begin()->second[0]^=std::byte{1};rejects([&]{b.prepare_authority(bad,au);});
    bad=saved;bad.detonation_serial=UINT64_MAX;rejects([&]{b.prepare_authority(bad,au);});
    auto wrong_units=au;wrong_units[0].source_datum^=0x10000u;rejects([&]{b.prepare_authority(saved,wrong_units);});
    auto other_constants=constants;other_constants.rest_speed_squared_bits^=1;NativeGrenadeSandbox other(bc,c.player.datum,other_constants);
    rejects([&]{other.prepare_authority(saved,au);});
    auto prepared=b.prepare_authority(saved,au);bu=au;bl=al;
    check(b.commit_authority(std::move(prepared)) && !b.commit_authority(std::move(prepared)),"Coupled grenade publication failed/reused token");
    same(saved,b.authority_checkpoint(bu));
    const auto ae=a.take_events(),be=b.take_events();check(ae.size()==be.size(),"Pending grenade events lost on import");
    for(unsigned i=0;i<ae.size();++i)same(ae[i],be[i]);check(b.take_events().empty(),"Imported grenade events replayed");
    CollisionWorld world(c.collision);unsigned ad=0,bd=0,ar=0,br=0;bool released=false;
    for(++tick;tick<400;++tick) {
        a.update_unit(au[0],false,0,[&](auto kind){return al.reserve(kind);},{},[&](auto h,auto d){al.publish_native(h,d);});
        b.update_unit(bu[0],false,0,[&](auto kind){return bl.reserve(kind);},{},[&](auto h,auto d){bl.publish_native(h,d);});
        a.update_projectiles(tick,world,[&](const auto &,Vec3,auto player,auto unit,auto designator,auto owner,auto){
            check(player==u.source_player && unit==u.source_datum && designator==2 && owner==0,"Actual grenade blast attribution changed");++ad;},
            [&](auto h){al.release(h);++ar;},[&](std::size_t p){return std::uint8_t(c.objects.at(p).type);});
        b.update_projectiles(tick,world,[&](const auto &,Vec3,auto,auto,auto,auto,auto){++bd;},
            [&](auto h){bl.release(h);++br;},[&](std::size_t p){return std::uint8_t(c.objects.at(p).type);});
        for(const auto &p:a.projectiles())released|=p.parent==null_datum;
        same(au[0],bu[0]);same(a.authority_checkpoint(au),b.authority_checkpoint(bu));
        check(std::ranges::equal(al.events(),bl.events()),"Grenade allocation/retirement history diverged");
        if(ad && a.projectiles().empty() && !(u.source_action_word&0x400000u))break;
    }
    check(released && ad==1 && bd==1 && ar==1 && br==1 && !a.detonations().empty(),"Actual native fuse/retirement did not finish once");
    std::cout<<"PASS actual native grenade type="<<type<<" held attachment/caches/pending events, release/flight/fuse/attribution/retirement across "<<tick<<" steps\n";
}
void source_bridge(const char *map,const char *xbe) {
    auto c=cache(map,xbe);const auto content=c->load_content(0,true);const auto constants=*c->original_data()->grenade_constants;
    NativeGrenadeSandbox a(c,content.player.datum,constants),b(c,content.player.datum,constants);
    std::array<NativeGrenadeSandbox::PresentationOwner,0> owners{};
    // Explicit completed-source input fixture, not an actual source-world
    // continuation. The root must supply that world before qualification.
    const auto globals=c->load_source_tag_metadata(c->globals_tag().value);
    const Bytes global(globals.bytes_at(globals.source_address(),globals.size()));
    check(global.u32(0x100)>=2,"Actual grenade globals unavailable");
    const auto definition=Bytes(globals.bytes_at(global.u32(0x104)+0x2cu,0x2c)).u32(0x28);
    NativeGrenadeProjectile p;p.datum=0xe1000001;p.definition=definition;p.type=1;p.unit=0xe1000002;p.player=0xd1230000;p.parent=0xe1000003;
    p.position={1,2,3};p.forward={1,0,0};p.up={0,0,1};
    a.adopt_source_projectiles({p},owners);rejects([&]{a.authority_checkpoint({});});
    SourceProjectileDetonationTu15 event{};event.projectile=p.datum;event.definition=p.definition;event.effect=123;
    event.effect_instance=0xe2000004;event.player=p.player;event.unit=p.unit;event.controls={.25f,.75f};event.colors={0x12345678,0x87654321};
    Transform frame;frame.position={4,5,6};event.effect_locations.push_back({42,frame});
    for(unsigned i=0;i<70;++i)a.adopt_source_detonations(100+i,std::span(&event,1));
    const auto saved=a.authority_checkpoint({},true);check(saved.detonations.size()==64 && saved.detonation_serial==70,"Source journal retention/frontier changed");
    rejects([&]{b.prepare_authority(saved,{});});auto staged=b.prepare_authority(saved,{},true);
    check(b.commit_authority(std::move(staged)),"Qualified source grenade presentation import failed");same(saved,b.authority_checkpoint({},true));
    auto malformed=event;malformed.effect_locations.clear();rejects([&]{b.adopt_source_detonations(200,std::span(&malformed,1));});
    rejects([&]{b.authority_checkpoint({},true);});staged=b.prepare_authority(saved,{},true);check(b.commit_authority(std::move(staged)),"Interrupted source bridge did not recover");
    b.adopt_source_detonations(200,std::span(&event,1));check(b.detonations().back().serial==71,"Imported journal serial restarted");
    std::cout<<"PASS declared completed-source presentation input, location/color/control preservation, qualification gate, 64-entry retention and interrupted-bridge recovery\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;check(argc==3,"Supply MAP and TU1.5 XBE");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    native(argv[1],argv[2],0);native(argv[1],argv[2],1);source_bridge(argv[1],argv[2]);return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
