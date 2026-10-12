#include "halo2/source_device_world.hpp"
#include "halo2/simulation.hpp"
#include "halo2/math.hpp"
#include "source_device_snapshot_check.hpp"
#include <bit>
#include <iostream>
#include <sstream>
using namespace halo2;
namespace {
void require(bool v,const char*text){if(!v)throw FormatError(text);}
void snapshots(){
    Cache cache("assets/disc/maps/zanzibar.map");const auto content=cache.load_content(0,true);
    SceneObjects scene(content,0);SourceDeviceWorld host(content);host.synchronize(scene);
    std::uint32_t control=~0u,gate=~0u;int gate_name=-1;
    for(const auto&[h,o]:host.owners()){
        if(content.devices[o.placement]->control)control=h;
        if(content.objects[o.placement].name>=0 && content.devices[o.placement]->machine &&
           content.devices[o.placement]->placement.position_group==0){gate=h;gate_name=content.objects[o.placement].name;}
    }
    require(control!=~0u && gate!=~0u,"snapshot authored control/gate missing");
    host.activate_admitted_control(control);for(unsigned i=0;i<180;++i)host.step(tick_seconds);
    const auto baseline=host.snapshot(180);
    SceneObjects unrelated(content,0);unrelated.destroy_all();
    SourceDeviceWorld replica(content);replica.synchronize(unrelated);replica.apply_snapshot(baseline);
    test::same_device_snapshot(baseline,replica.snapshot(0));
    require(!replica.synchronize(unrelated) && replica.find(gate),"client scene allocator replaced authoritative device identities");
    require(replica.synchronize_collision(),"replicated pose did not publish collision");
    const auto collision_count=replica.triangles().size();require(collision_count>0,"replica lost authored collision");
    replica.apply_snapshot(baseline);require(!replica.synchronize_collision(),"identical snapshot rebuilt device collision");
    const auto reject=[&](SourceDeviceWorldSnapshot bad){
        bool threw=false;try{replica.apply_snapshot(bad);}catch(const FormatError&){threw=true;}
        require(threw,"malformed device bank accepted");test::same_device_snapshot(baseline,replica.snapshot(0));
        require(replica.triangles().size()==collision_count,"malformed device bank mutated collision");
    };
    auto bad=baseline;bad.groups.pop_back();reject(bad);
    bad=baseline;bad.owners[0].definition^=1;reject(bad);
    bad=baseline;bad.owners[1].state.handle=bad.owners[0].state.handle;reject(bad);
    bad=baseline;bad.owners[1].state.handle=bad.owners[0].state.handle^0x10000u;reject(bad);
    bad=baseline;bad.owners[0].state.created_tick=181;reject(bad);
    bad=baseline;bad.tick=181;bad.owners[0].state.created_tick=1;reject(bad);
    bad=baseline;bad.owners[0].state.motion.power=std::bit_cast<float>(0x7fc00000u);reject(bad);
    bad=baseline;bad.owners[0].state.bsp_hidden=true;bad.owners[0].state.visible=true;reject(bad);
    bool denied=false;try{replica.step(tick_seconds);}catch(const FormatError&){denied=true;}
    require(denied,"device replica advanced a private clock");denied=false;
    try{replica.activate_admitted_control(control);}catch(const FormatError&){denied=true;}
    require(denied,"device replica wrote an authority group");
    scene.destroy(gate);host.synchronize(scene);replica.apply_snapshot(host.snapshot(181));
    require(!replica.find(gate) && replica.groups()[0].flags==3,"device deletion reset the shared one-use group");
    const auto replacement=scene.create(gate_name,false,182);host.synchronize(scene);
    replica.apply_snapshot(host.snapshot(182));require(replacement!=gate && replica.find(replacement) && !replica.find(gate),
        "recreated device generation was not replicated");
    require(replica.find(replacement)->motion.position==1,"replicated recreation used authored initial group instead of current value");
    denied=false;try{replica.apply_snapshot(baseline);}catch(const FormatError&){denied=true;}
    require(denied && replica.find(replacement),"stale snapshot rolled back a recreated device");
    scene.visible(replacement,false);host.synchronize(scene);const auto hidden=host.snapshot(182);
    replica.apply_snapshot(hidden);require(!replica.find(replacement)->visible,"same-tick visibility change lost");
    test::same_device_snapshot(hidden,replica.snapshot(0));
    SourceDeviceWorld reset(content),new_peer(content);SceneObjects reset_scene(content,0);reset.synchronize(reset_scene);
    new_peer.apply_snapshot(reset.snapshot(0));require(new_peer.groups()[0].value==0 && new_peer.groups()[0].flags==1,
        "new-map replica retained previous one-use state");
    // The original gear reducer permits a negative phase: it wraps only the
    // upper boundary once. A native transport bank must preserve finite
    // source values rather than adding a channel/group clamp of its own.
    auto finite_bank=baseline;SourceMachineDefinition reverse_gear;
    reverse_gear.type=2;reverse_gear.powered_position_rate=-1;
    const auto phase=source_machine_gear_step(reverse_gear,0,1,.25f).position;
    require(phase==-.25f,"source reverse gear fixture changed");
    finite_bank.owners[0].state.motion.position=phase;
    finite_bank.owners[0].state.private_position.value=phase;finite_bank.groups[0].value=phase;
    SourceDeviceWorld finite_peer(content);finite_peer.apply_snapshot(finite_bank);
    test::same_device_snapshot(finite_bank,finite_peer.snapshot(0));
    std::cout<<"Device snapshots: exact state, allocator independence, atomic rejection, deletion/recreation, stale/same-tick and fresh-map baselines passed\n";
}
void all_maps(){
    const std::array<std::pair<std::string,std::string>,2> lists{{
        {"assets/disc/maps/","ascension beavercreek burial_mounds coagulation colossus cyclotron foundation headlong lockout midship waterworks zanzibar"},
        {"assets/map-pack/maps/","backwash containment deltatap dune elongation gemini triplicate turf warlock"}}};
    unsigned maps=0,owners=0;
    for(const auto&[directory,names]:lists){std::istringstream in(names);std::string name;
        while(in>>name){
            Cache cache(directory+name+".map");const auto content=cache.load_content(0,true);
            SceneObjects scene(content,0);SourceDeviceWorld devices(content);devices.synchronize(scene);
            owners+=unsigned(devices.owners().size());devices.synchronize_collision();
            devices.step(tick_seconds);devices.synchronize_collision();
            const auto packet=devices.snapshot(1);SourceDeviceWorld replica(content);replica.apply_snapshot(packet);
            test::same_device_snapshot(packet,replica.snapshot(0));replica.synchronize_collision();
            require(replica.triangles().size()==devices.triangles().size(),"all-map replica lost authored device collision");++maps;
        }
    }
    require(maps==21 && owners==53,"all-map initial simulation device owner coverage changed");
    std::cout<<"21 Xbox maps: 53 admitted machine/control owners, common/gear tick and posed collision passed\n";
}
void map(const char*path,unsigned expected_controls){
    Cache cache(path);const auto content=cache.load_content(0,true);
    SceneObjects scene(content,0);SourceDeviceWorld devices(content);devices.synchronize(scene);
    devices.synchronize_collision();
    std::vector<std::uint32_t> controls;
    for(const auto&[handle,o]:devices.owners())if(content.devices[o.placement]->control)controls.push_back(handle);
    require(controls.size()==expected_controls,"authored live control admission changed");
    for(const auto handle:controls){
        const auto*control=devices.find(handle);const auto placement=control->placement;
        const auto&definition=*content.devices[placement];
        const auto group=definition.placement.position_group;
        require(group>=0,"expected shared authored control group");
        const auto initial=devices.groups()[unsigned(group)];
        require(initial.value==0,"authored control group initial value changed");
        // Explicit unit-center/bounds/posed-marker services exercise only
        // state binding here. These inputs do not claim live CAFC0/B8D30.
        SourceControlTargetQuery query;query.unit_bound_radius=.5f;query.unit_forward={1,0,0};
        SourceControlTargetCandidate candidate;candidate.object=handle;
        candidate.bound_center={.5f,0,0};candidate.bound_radius=.2f;
        candidate.marker_count=1;candidate.marker_forward={-1,0,0};candidate.root={.5f,0,0};
        SourcePlayerActionTarget target;
        require(devices.consider_control_target(query,candidate,target,{}) && target.object==handle && target.action==3,
            "live shared control state did not bind to source target gate");
        const auto first=devices.activate_admitted_control(handle);
        require(first.changed && devices.groups()[unsigned(group)].value==1,"admitted control did not update shared target");
        target={};
        require(devices.consider_control_target(query,candidate,target,{})==!(initial.flags&1u),
            "one-use group selection did not consume actual shared changed flag");
        const auto retained=target;candidate.object^=0x10000u;
        require(!devices.consider_control_target(query,candidate,target,{}) && target==retained,
            "stale control generation replaced the selected action");
        std::vector<std::uint32_t> linked;
        for(const auto&[id,o]:devices.owners())if(content.devices[o.placement]->machine &&
            content.devices[o.placement]->placement.position_group==group){
            linked.push_back(id);require(o.motion.position==0,"control teleported a linked machine");
        }
        require(!linked.empty(),"control lost authored linked machine");
        std::vector<CollisionTriangle> closed;
        if(std::string_view(path).find("zanzibar")!=std::string_view::npos){
            const auto placement=devices.find(linked.front())->placement;
            for(const auto&t:devices.triangles())if(t.source.instance==int(placement))closed.push_back(t);
            require(!closed.empty(),"Zanzibar gate has no authored collision consumer");
        }
        devices.step(tick_seconds);
        for(const auto id:linked){const auto*o=devices.find(id);
            const auto&d=content.devices[o->placement]->motion;
            if(d.position_rate>0 && d.delay==0)
                require(o->motion.position>0 && o->motion.position<1,"linked machine has no source controller progression");
        }
        const auto second=devices.activate_admitted_control(handle);
        require(!second.changed,"authored on/one-use control unexpectedly changed on repeat");
        require(devices.groups()[unsigned(group)].flags==(initial.flags|2),"shared group changed flags lost");
        for(unsigned i=1;i<300;++i)devices.step(tick_seconds);
        devices.synchronize_collision();
        if(!closed.empty()){
            const auto placement=devices.find(linked.front())->placement;
            std::vector<CollisionTriangle> opened;
            for(const auto&t:devices.triangles())if(t.source.instance==int(placement))opened.push_back(t);
            require(opened.size()==closed.size(),"opening gate discarded its collision surfaces");
            CollisionWorld before(closed),after(opened);bool cleared=false;
            for(const auto&t:closed){
                const auto center=(t.a+t.b+t.c)*(1.f/3.f);
                const auto normal=normalized(t.source_normal);const auto origin=center-normal*.25f;
                if(before.raycast(origin,normal,.5f) && !after.raycast(origin,normal,.5f)){cleared=true;break;}
            }
            require(cleared,"source gate motion did not clear any originally blocked path");
        }
        for(const auto id:linked){const auto*o=devices.find(id);
            if(content.devices[o->placement]->motion.position_rate>0)
                require(o->motion.position==1 && o->motion.position_velocity==0,"linked machine did not settle at authored endpoint");
        }
        // Recreate a full generation using the current shared target and
        // flags. The first generation cannot be used after destruction.
        auto recreated=handle;auto name=content.objects[placement].name;
        if(name<0)for(const auto id:linked){
            const auto candidate=content.objects[devices.find(id)->placement].name;
            if(candidate>=0){name=candidate;recreated=id;break;}
        }
        require(name>=0,"shared group has no authored named lifecycle fixture");
        scene.destroy(recreated);devices.synchronize(scene);require(!devices.find(recreated),"stale device survived scene destruction");
        const auto replacement=scene.create(name,false,300);devices.synchronize(scene);
        require(replacement!=recreated && devices.find(replacement)->created_tick==300,"device generation did not reset");
        require(devices.find(replacement)->motion.position==1,"recreated control reset the shared group");
        require(!devices.activate_admitted_control(recreated==handle?replacement:handle).changed,
            "recreated one-use/on device reactivated group");
    }
    // A fresh simulation owns device progression. Rendering is not required
    // to advance a private gear, including when no local view is submitted.
    if(std::string_view(path).find("zanzibar")!=std::string_view::npos){
        Spawn spawn;spawn.position={0,0,10};Simulation simulation(content,0,spawn);
        std::uint32_t gear=~0u;
        for(const auto&[id,o]:simulation.devices().owners())if(content.devices[o.placement]->machine &&
            content.devices[o.placement]->machine->type==2){gear=id;break;}
        require(gear!=~0u,"Zanzibar gear missing");
        const auto before=simulation.devices().find(gear)->motion.position;
        simulation.step({});
        const auto after=simulation.devices().find(gear)->motion.position;
        require(after!=before,"simulation did not own the device tick");
        simulation.request_checkpoint();simulation.step({});
        const auto saved=simulation.devices().find(gear)->motion.position;
        simulation.step({});simulation.step({});simulation.request_revert();simulation.step({});
        require(std::bit_cast<unsigned>(simulation.devices().find(gear)->motion.position)==std::bit_cast<unsigned>(saved),
            "checkpoint did not restore source device state");
    }
    std::cout<<path<<": shared controls, source motion, one-use flags, generation reset passed\n";
}
}
int main(){try{
    const std::vector<SourceDeviceWorldEvent> events{{0x10001,0x10002,1,2},{0x20003,0x20004,2,3}};
    validate_source_device_events(events,3);
    NativeDeviceEventCursor authority(true),late(false);
    require(authority.observe(events).size()==2 && authority.observe(events).empty(),"Native authority event cursor duplicated requests");
    require(late.observe(events).empty(),"Native late join replayed retained requests");
    const auto reject=[&](auto bad){bool threw=false;try{validate_source_device_events(bad,3);}catch(const FormatError&){threw=true;}
        require(threw,"Invalid native device event journal accepted");};
    auto bad=events;bad[0].serial=0;reject(bad);bad=events;bad[1].serial=1;reject(bad);
    bad=events;bad[1].tick=4;reject(bad);bad=events;bad[0].handle=0x10800;reject(bad);
    bad=events;bad[0].handle=1;reject(bad);bad=events;bad[0].tag=null_datum;reject(bad);
    bad.assign(65,events[0]);reject(bad);
    map("assets/disc/maps/zanzibar.map",1);map("assets/map-pack/maps/containment.map",2);map("assets/map-pack/maps/dune.map",1);
    all_maps();snapshots();
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
