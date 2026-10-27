#include "halo2/source_device_world.hpp"
#include "halo2/scene.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "source_device_snapshot_check.hpp"
#include <algorithm>
#include <bit>
#include <iostream>
#include <limits>
#include <tuple>

using namespace halo2;
namespace {
using Checkpoint=SourceDeviceWorld::AuthorityCheckpoint;
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}check(failed,"Invalid/active device continuation accepted");}
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
bool same(const Transform &a,const Transform &b){
    if(!same(a.scale,b.scale)||!same(a.position,b.position))return false;
    for(unsigned i=0;i<3;++i)if(!same(a.basis[i],b.basis[i]))return false;return true;
}
bool same(const CollisionTriangle &a,const CollisionTriangle &b){return same(a.a,b.a)&&same(a.b,b.b)&&same(a.c,b.c)&&
    same(a.source_normal,b.source_normal)&&a.material==b.material&&a.flags==b.flags&&
    std::tie(a.source.bsp,a.source.record,a.source.surface,a.source.instance,a.source.plane)==
    std::tie(b.source.bsp,b.source.record,b.source.surface,b.source.instance,b.source.plane);}
void same(std::span<const CollisionTriangle> a,std::span<const CollisionTriangle> b){
    check(a.size()==b.size(),"Device cache triangle count differs");
    for(unsigned i=0;i<a.size();++i)check(same(a[i],b[i]),"Device cache values/order/source identities differ");
}
void same(std::span<const SourceDeviceWorldEvent> a,std::span<const SourceDeviceWorldEvent> b){
    check(a.size()==b.size(),"Device pending request count differs");
    for(unsigned i=0;i<a.size();++i)check(std::tie(a[i].handle,a[i].tag,a[i].serial,a[i].tick)==
        std::tie(b[i].handle,b[i].tag,b[i].serial,b[i].tick),"Device pending request identity/order differs");
}
void same(const Checkpoint &a,const Checkpoint &b){
    test::same_device_snapshot(a.state,b.state);
    check(a.version==b.version && a.definition_binding==b.definition_binding && a.collision_dirty==b.collision_dirty &&
        a.collision_poses.size()==b.collision_poses.size(),"Device continuation binding/cache frontier differs");
    for(const auto &[h,p]:a.collision_poses){const auto &q=b.collision_poses.at(h);
        check(std::tie(p.handle,p.placement,p.created_tick,p.bsp_hidden)==std::tie(q.handle,q.placement,q.created_tick,q.bsp_hidden) &&
            same(p.root,q.root)&&same(p.position,q.position)&&same(p.power,q.power),"Device cached pose provenance differs");}
    same(a.triangles,b.triangles);same(a.pending_events,b.pending_events);
}
void real(const char *xbe,const char *map){
    Cache ac(map,{},OriginalData::load_xbe(xbe)),bc(map,{},OriginalData::load_xbe(xbe));
    auto acontent=ac.load_content(0,true),bcontent=bc.load_content(0,true);
    SceneObjects as(acontent,0,SourceSceneGameContext{2,2}),bs(bcontent,0,SourceSceneGameContext{2,2});
    SourceDeviceWorld a(acontent),b(bcontent);a.synchronize(as);b.synchronize(bs);
    check(a.synchronize_collision()&&b.synchronize_collision(),"Actual device cache was not built");
    const auto clean=a.authority_checkpoint(0);
    check(!clean.triangles.empty()&&!clean.collision_poses.empty()&&!clean.collision_dirty,"Actual clean cache unavailable");
    b.restore_authority(clean,0);same(clean,b.authority_checkpoint(0));check(!b.synchronize_collision(),"Clean import rebuilt collision early");
    std::uint32_t control=null_datum,gate=null_datum;std::size_t placement=0;
    for(const auto &[h,o]:a.owners()){
        const auto &d=*acontent.devices[o.placement];
        if(d.control)control=h;
        if(d.machine && d.placement.position_group==0 && acontent.objects[o.placement].name>=0){gate=h;placement=o.placement;}
    }
    check(control!=null_datum&&gate!=null_datum,"Actual admitted Zanzibar control/gate unavailable");
    check(acontent.devices[placement].get()!=bcontent.devices[placement].get(),"Device target reused source definition allocation");
    a.activate_admitted_control(control);
    for(unsigned i=0;i<17;++i)a.step(1.f/30.f);
    const auto dirty=a.authority_checkpoint(17);
    check(dirty.collision_dirty&&!dirty.pending_events.empty(),"Actual motion did not leave a dirty cache/pending requests");
    same(clean.triangles,dirty.triangles);
    check(!same(dirty.collision_poses.at(gate).position,a.find(gate)->motion.position),"Fixture did not retain an older collision pose");
    {auto staged=b.prepare_authority(dirty,17);
        rejects([&]{b.step(1.f/30.f);});rejects([&]{b.synchronize(bs);});rejects([&]{b.synchronize_collision();});
        rejects([&]{b.script_set(gate,false,.25f,true);});rejects([&]{b.apply_snapshot(dirty.state);});
        rejects([&]{b.take_events();});rejects([&]{b.authority_checkpoint(17);});rejects([&]{b.prepare_authority(dirty,17);});
        rejects([&]{auto copy=b;(void)copy;});rejects([&]{auto copy=a;copy=b;});
        check(!a.commit_authority(std::move(staged)),"Foreign device token published");}
    same(clean,b.authority_checkpoint(0));
    auto bad=dirty;bad.version=1;rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.definition_binding[0]^=std::byte{1};rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.state.groups.pop_back();rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.triangles.clear();
    bool preflight_rejected{};
    try{b.prepare_authority(bad,17);}catch(const FormatError &e){
        preflight_rejected=std::string_view(e.what()).find("admitted triangle budget")!=std::string_view::npos;
    }
    check(preflight_rejected,"Device generated collision allocated before checking the admitted triangle extent");
    bad=dirty;bad.triangles[0].a.x+=.125f;rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;std::reverse(bad.triangles.begin(),bad.triangles.end());rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.collision_poses.at(gate).created_tick=18;rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.collision_poses.at(gate).root.scale=std::numeric_limits<float>::infinity();rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.collision_dirty=false;rejects([&]{b.prepare_authority(bad,17);});
    bad=dirty;bad.pending_events[0].handle=1;rejects([&]{b.prepare_authority(bad,17);});
    rejects([&]{b.prepare_authority(dirty,18);});
    auto changed=bcontent;auto definition=std::make_shared<SourceDeviceWorldDefinition>(*changed.devices[placement]);
    definition->motion.position_rate+=.125f;changed.devices[placement]=definition;SourceDeviceWorld wrong(changed);
    rejects([&]{wrong.prepare_authority(dirty,17);});same(clean,b.authority_checkpoint(0));
    const auto publish=[&](std::uint64_t tick){
        const auto scene=as.authority_checkpoint();const auto saved=a.authority_checkpoint(tick);
        auto scene_stage=bs.prepare_authority(scene);auto stage=b.prepare_authority(saved,tick);
        check(bs.commit_authority(std::move(scene_stage))&&b.commit_authority(std::move(stage))&&!b.commit_authority(std::move(stage)),
            "Composed device/scene publication failed or reused token");
        same(saved,b.authority_checkpoint(tick));
    };
    publish(17);same(clean.triangles,b.triangles());
    same(a.take_events(),b.take_events());check(b.take_events().empty(),"Imported device requests replayed");
    check(a.synchronize_collision()&&b.synchronize_collision(),"Dirty cache rebuild frontier was lost");
    same(a.triangles(),b.triangles());
    for(unsigned tick=18;tick<=77;++tick){
        a.step(1.f/30.f);b.step(1.f/30.f);same(a.authority_checkpoint(tick),b.authority_checkpoint(tick));
        check(a.synchronize_collision()==b.synchronize_collision(),"Future device rebuild schedule differs");
        same(a.triangles(),b.triangles());same(a.take_events(),b.take_events());
    }
    // Dirty cache can legitimately retain a retired full generation until
    // the ordinary rebuild. Scene and device banks publish under one root.
    as.destroy(gate);a.synchronize(as);const auto retired=a.authority_checkpoint(78);
    check(retired.collision_dirty&&retired.collision_poses.contains(gate)&&!a.find(gate),"Retired cache pose was not retained");
    publish(78);check(a.synchronize_collision()&&b.synchronize_collision(),"Retired collision was not rebuilt once");
    const auto replacement=as.create_placement(placement,79);check(replacement!=gate,"Actual scene reused old full salt");
    a.synchronize(as);publish(79);check(b.find(replacement)&&!b.find(gate),"Recreated device generation was lost");
    check(a.synchronize_collision()&&b.synchronize_collision(),"New device generation did not rebuild collision");
    same(a.authority_checkpoint(79),b.authority_checkpoint(79));
    // Only the replacement is used after source definition allocations retire.
    acontent.devices.clear();b.step(1.f/30.f);b.synchronize_collision();b.authority_checkpoint(80);
    std::cout<<"PASS actual Zanzibar/TU1.5 device dirty/clean cache, exact collision order/provenance, sixty future motion/event steps, deletion/recreation, local definitions and staged admission\n";
}
void interrupted(){
    // Explicit native door/query fixture. It tests transaction admission, not
    // an authored Zanzibar automatic door or complete original scene physics.
    LevelContent c;c.bsp_count=1;c.object_names={{"door",7,0}};ScenarioObject object;
    object.definition=0x12340001;object.type=7;object.name=0;object.source=-1;object.bounding_radius=1;c.objects.push_back(object);
    auto d=std::make_shared<SourceDeviceWorldDefinition>();d->machine.emplace();d->machine->type=0;
    d->machine->door.activation_radius=1;d->machine->door.open_seconds=2;d->placement.initial_power=1;
    d->placement.machine_flags=4;d->motion.position_rate=1;d->motion.position_acceleration=1;c.devices.push_back(d);
    SceneObjects scene(c,0);SourceDeviceWorld a(c),b(c);a.synchronize(scene);a.synchronize_collision();
    const auto saved=a.authority_checkpoint(0);const auto handle=a.owners().begin()->first;
    SourceDeviceWorld::DoorWorld world;world.tick=(0u-handle)&3u;unsigned callbacks=0;
    world.query=[&](auto,Vec3,float)->std::span<const SourceMachineDoorUnit>{
        ++callbacks;rejects([&]{a.authority_checkpoint(0);});rejects([&]{auto copy=a;(void)copy;});
        throw FormatError("Declared door query interruption");};
    rejects([&]{a.step(1.f/30.f,world);});check(callbacks==1,"Declared callback frontier was not reached");
    rejects([&]{a.authority_checkpoint(0);});a.restore_authority(saved,0);b.restore_authority(saved,0);
    world.query=[](auto,Vec3,float)->std::span<const SourceMachineDoorUnit>{return {};};
    a.step(1.f/30.f,world);b.step(1.f/30.f,world);same(a.authority_checkpoint(1),b.authority_checkpoint(1));
    a.script_set(handle,false,1,true);
    for(unsigned tick=2;tick<=9;++tick){world.tick=tick;a.step(1.f/30.f,world);}
    const auto timer=a.authority_checkpoint(9);check(a.find(handle)->door.flags==16&&a.find(handle)->door.open_ticks==8,
        "Declared automatic-door timer/flags fixture did not reach its retained frontier");
    b.restore_authority(timer,9);
    for(unsigned tick=10;tick<=80;++tick){world.tick=tick;a.step(1.f/30.f,world);b.step(1.f/30.f,world);
        same(a.authority_checkpoint(tick),b.authority_checkpoint(tick));}
    check(b.position_group(*b.find(handle)).value==0,"Imported automatic-door timeout failed to request close");
    std::cout<<"PASS declared automatic-door query capture rejection, interrupted-call recovery and seventy-one future timer/close steps\n";
}
}
int main(int argc,char **argv)try {
    std::cout<<std::unitbuf;check(argc==3,"Supply TU1.5 XBE and Zanzibar MAP");
    if(!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
    real(argv[1],argv[2]);interrupted();return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
