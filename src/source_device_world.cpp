#include "halo2/source_device_world.hpp"
#include "halo2/content.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_portal_mask.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/math.hpp"
#include "halo2/render_interpolation.hpp"
#include <cstring>
#include <algorithm>
#include <set>
#include <utility>
#include <cmath>
#include <limits>

namespace halo2 {
void validate_source_device_events(std::span<const SourceDeviceWorldEvent> events,std::uint64_t tick){
    if(events.size()>64)throw FormatError("native device event journal exceeds retention");
    std::uint64_t previous=0;
    for(const auto &e:events){
        if(!e.serial || e.serial<=previous || e.tick>tick || e.tag==null_datum ||
           e.handle==null_datum || !(e.handle>>16) || (e.handle&0xffffu)>=2048)
            throw FormatError("invalid native device event journal");
        previous=e.serial;
    }
}
std::vector<SourceDeviceWorldEvent> NativeDeviceEventCursor::observe(std::span<const SourceDeviceWorldEvent> events){
    std::vector<SourceDeviceWorldEvent> result;
    for(const auto &e:events)if(e.serial>serial_){serial_=e.serial;if(baseline_)result.push_back(e);}
    baseline_=true;return result;
}
namespace {
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
bool valid_group(const SourceDeviceGroup&g){return std::isfinite(g.value);}
bool same_pose(const SourceDeviceWorldOwner&a,const SourceDeviceWorldOwner&b){
    static_assert(sizeof(Transform)==13*sizeof(float));
    return a.placement==b.placement && a.created_tick==b.created_tick && a.bsp_hidden==b.bsp_hidden &&
        std::memcmp(&a.root,&b.root,sizeof(Transform))==0 &&
        std::memcmp(&a.motion.position,&b.motion.position,sizeof(float))==0 &&
        std::memcmp(&a.motion.power,&b.motion.power,sizeof(float))==0;
}
}
void validate_source_device_world_snapshot(const SourceDeviceWorldSnapshot &s){
    if(s.groups.size()>65536 || s.owners.size()>2048)
        throw FormatError("native device snapshot exceeds source bank capacity");
    for(const auto&g:s.groups)if(!valid_group(g))throw FormatError("invalid native device group value");
    std::set<std::uint32_t> handles,slots;std::set<std::size_t> placements;
    for(const auto&entry:s.owners){const auto&o=entry.state;
        if(entry.definition==null_datum || o.handle==null_datum || !(o.handle>>16) ||
           (o.handle&0xffffu)>=2048 || o.placement>std::numeric_limits<std::uint32_t>::max() ||
           o.created_tick>s.tick || !handles.insert(o.handle).second ||
           !slots.insert(o.handle&0xffffu).second || !placements.insert(o.placement).second)
            throw FormatError("invalid native device snapshot identity");
        // Transport preserves finite source values. In particular,1152E0's
        // gear path performs a single upper wrap and no lower clamp; this
        // adapter cannot impose an extra [0,1] invariant on its channels or
        // direct group writes.
        if(!std::isfinite(o.motion.position)||!std::isfinite(o.motion.power)||
           !std::isfinite(o.motion.position_velocity)||!std::isfinite(o.motion.power_velocity)||
           !valid_group(o.private_power)||!valid_group(o.private_position)||
           !std::isfinite(o.root.scale)||o.root.scale<=0 || !finite(o.root.position))
            throw FormatError("invalid native device snapshot channels/transform");
        for(const auto&axis:o.root.basis)if(!finite(axis))
            throw FormatError("invalid native device snapshot basis");
        if(o.bsp_hidden && o.visible)throw FormatError("hidden native device snapshot submits geometry");
    }
}
SourceDeviceWorld::SourceDeviceWorld(const LevelContent &c):content_(&c),groups_(c.device_groups){}
SourceDeviceGroup &SourceDeviceWorld::group(SourceDeviceWorldOwner &owner,bool power){
    const auto&d=content_->devices.at(owner.placement)->placement;
    const auto index=power?d.power_group:d.position_group;
    if(index==-1)return power?owner.private_power:owner.private_position;
    if(index<0 || std::size_t(index)>=groups_.size())throw FormatError("device owner has invalid shared group");
    return groups_[unsigned(index)];
}
const SourceDeviceGroup &SourceDeviceWorld::position_group(const SourceDeviceWorldOwner&o)const{
    const auto index=content_->devices.at(o.placement)->placement.position_group;
    return index==-1?o.private_position:groups_.at(unsigned(index));
}
const SourceDeviceGroup &SourceDeviceWorld::power_group(const SourceDeviceWorldOwner&o)const{
    const auto index=content_->devices.at(o.placement)->placement.power_group;
    return index==-1?o.private_power:groups_.at(unsigned(index));
}
const SourceDeviceWorldOwner *SourceDeviceWorld::find(std::uint32_t handle)const{
    const auto at=owners_.find(handle);return at==owners_.end()?nullptr:&at->second;
}
bool SourceDeviceWorld::synchronize(const SceneObjects &scene){
    if(replicated())return false; // Client scene allocations are a different owner.
    bool changed=false;std::set<std::uint32_t> present;
    for(const auto&o:scene.objects()){
        if(o.handle==null_datum || o.placement>=content_->devices.size() || !content_->devices[o.placement])continue;
        present.insert(o.handle);
        auto [at,fresh]=owners_.try_emplace(o.handle);auto&owner=at->second;
        if(fresh){
            const auto&d=content_->devices[o.placement]->placement;
            owner.handle=o.handle;owner.placement=o.placement;owner.created_tick=o.created_tick;
            owner.private_power={d.initial_power,d.power_group_flags};
            owner.private_position={d.initial_position,d.position_group_flags};
            //1076E0 copies CURRENT named groups on each construction. A
            // destroyed/recreated switch cannot reset another object's group.
            owner.motion.power=group(owner,true).value;owner.motion.position=group(owner,false).value;
            owner.motion.flags=source_machine_initial_device_flags(d.device_flags);
            if(content_->devices[o.placement]->control)
                owner.control_flags=source_control_initial_flags(d.machine_flags);
            changed=true;
        }else if(owner.placement!=o.placement || owner.created_tick!=o.created_tick)
            throw FormatError("device full generation changed its source identity");
        const auto root=scene_object_transform(o);
        changed|=std::memcmp(&root,&owner.root,sizeof root)!=0;owner.root=root;
        changed|=owner.bsp_hidden!=o.bsp_hidden;owner.bsp_hidden=o.bsp_hidden;
        owner.visible=o.visible;
    }
    for(auto at=owners_.begin();at!=owners_.end();)if(!present.contains(at->first)){
        at=owners_.erase(at);changed=true;
    }else ++at;
    collision_dirty_|=changed;return changed;
}
void SourceDeviceWorld::step(float dt){
    if(replicated())throw FormatError("replicated devices cannot advance an independent client clock");
    // Source scans pool slots, so generation salts cannot reorder updates.
    std::vector<SourceDeviceWorldOwner*> order;for(auto&[_,o]:owners_)order.push_back(&o);
    std::sort(order.begin(),order.end(),[](const auto*a,const auto*b){return (a->handle&0xffff)<(b->handle&0xffff);});
    for(auto*ptr:order){auto&o=*ptr;if(o.bsp_hidden)continue;
        const auto&d=*content_->devices[o.placement];
        const float old_position=o.motion.position,old_power=o.motion.power;
        const auto result=source_device_motion_step(d.motion,o.motion,group(o,false).value,group(o,true).value,dt);
        for(const auto tag:result.effects)if(tag!=null_datum)events_.push_back({o.handle,tag});
        //106790 precedes1152E0. Gear groups write directly, without1071E0's
        //one-use/changed flag side effects.
        if(d.machine){
            if(d.machine->type==2){
                o.motion.position_velocity=0;
                const auto gear=source_machine_gear_step(*d.machine,o.motion.position,o.motion.power,dt);
                if(gear.changed){o.motion.position=gear.position;group(o,false).value=gear.position;}
            }
            //1152E0 consumes the transform-dirty bit. Automatic door proximity,
            //autoclose and child-physics consumers remain separate work.
            o.motion.flags&=~4u;
        }
        collision_dirty_|=old_position!=o.motion.position || old_power!=o.motion.power;
    }
}
bool SourceDeviceWorld::synchronize_collision(){
    if(!collision_dirty_)return false;
    triangles_.clear();
    for(const auto&[_,o]:owners_)if(!o.bsp_hidden){
        const auto&d=*content_->devices[o.placement];if(!d.collision)continue;
        const auto locals=source_machine_local_pose(d.collision->hierarchy,*d.machine,o.motion.position,o.motion.power);
        const auto nodes=pose_model_locals_nodes_world(d.collision->hierarchy,o.root,locals);
        auto triangles=model_collision_triangles(d.collision->collision,nodes,d.collision->permutations,o.placement);
        triangles_.insert(triangles_.end(),triangles.begin(),triangles.end());
    }
    collision_dirty_=false;return true;
}
SourceControlActivation SourceDeviceWorld::activate_admitted_control(std::uint32_t handle){
    if(replicated())throw FormatError("replicated devices cannot admit a client group write");
    auto at=owners_.find(handle);if(at==owners_.end())throw FormatError("control action has stale device owner");
    auto&o=at->second;const auto&d=*content_->devices[o.placement];
    if(!d.control)throw FormatError("control action does not name a control");
    if(d.control->triggers_when!=0)return {};
    const auto result=source_control_activate(*d.control,&group(o,false));
    if(result.effect!=null_datum)events_.push_back({handle,result.effect});
    return result;
}
bool SourceDeviceWorld::consider_control_target(const SourceControlTargetQuery &q,
    SourceControlTargetCandidate candidate,SourcePlayerActionTarget &current,Vec3 current_root)const{
    const auto*owner=find(candidate.object);
    if(!owner || owner->bsp_hidden || !content_->devices[owner->placement]->control)return false;
    candidate.position=&position_group(*owner);candidate.power=&power_group(*owner);
    candidate.device_flags=owner->motion.flags;candidate.control_flags=owner->control_flags;
    return source_control_target_candidate(q,candidate,current,current_root);
}
std::vector<SourceDeviceWorldEvent> SourceDeviceWorld::take_events(){return std::exchange(events_,{});}
SourceDeviceWorldSnapshot SourceDeviceWorld::snapshot(std::uint64_t tick)const{
    SourceDeviceWorldSnapshot result;result.tick=replica_tick_.value_or(tick);result.groups=groups_;
    for(const auto&[_,owner]:owners_)
        result.owners.push_back({content_->objects.at(owner.placement).definition,owner});
    validate_source_device_world_snapshot(result);return result;
}
void SourceDeviceWorld::apply_snapshot(const SourceDeviceWorldSnapshot &s){
    validate_source_device_world_snapshot(s);
    if(s.groups.size()!=content_->device_groups.size())throw FormatError("native device groups differ from this level");
    if(replica_tick_ && s.tick<*replica_tick_)throw FormatError("stale native device snapshot");
    std::map<std::uint32_t,SourceDeviceWorldOwner> next;
    bool pose_changed=!replica_tick_ || s.owners.size()!=owners_.size();
    for(const auto&entry:s.owners){const auto&o=entry.state;
        if(o.placement>=content_->devices.size() || !content_->devices[o.placement] ||
           o.placement>=content_->objects.size() || content_->objects[o.placement].definition!=entry.definition)
            throw FormatError("native device snapshot differs from authored placement");
        const auto&d=content_->devices[o.placement]->placement;
        for(const auto index:{d.power_group,d.position_group})if(index< -1 ||
            (index>=0 && std::size_t(index)>=s.groups.size()))
            throw FormatError("native device snapshot group link differs from this level");
        // A full handle remains one generation. A later constructor must
        // arrive with a different handle, even if its placement is the same.
        const auto*old=find(o.handle);
        if(replica_tick_ && old && (old->placement!=o.placement || old->created_tick!=o.created_tick))
            throw FormatError("native device snapshot reuses a full generation identity");
        pose_changed|=!old || !same_pose(*old,o);
        next.emplace(o.handle,o);
    }
    // No mutation above this point: malformed banks cannot consume a one-use
    // group or partially replace the current client owners.
    groups_=s.groups;owners_=std::move(next);replica_tick_=s.tick;
    events_.clear();collision_dirty_|=pose_changed;
}
float SourceDeviceWorld::script_value(std::uint32_t handle,bool power)const {
    if(handle==null_datum)return 0;const auto &owner=owners_.at(handle);
    return power?owner.motion.power:owner.motion.position;
}
void SourceDeviceWorld::script_immediate(SourceDeviceGroup &target,float value) {
    target.value=std::clamp(value,0.f,1.f);
    for(auto &[handle,owner]:owners_) {
        if(&group(owner,false)==&target){owner.motion.flags|=4;owner.motion.position=target.value;owner.motion.position_velocity=0;}
        if(&group(owner,true)==&target){owner.motion.flags|=4;owner.motion.power=target.value;owner.motion.power_velocity=0;}
    }
    collision_dirty_=true;
}
bool SourceDeviceWorld::script_set(std::uint32_t handle,bool power,float value,bool immediate) {
    if(replicated())throw FormatError("Client cannot script device state");
    if(!std::isfinite(value))throw FormatError("Nonfinite scripted device value");
    if(handle==null_datum)return false;auto &owner=owners_.at(handle);auto &target=group(owner,power);
    if(immediate) {
        if(power){script_immediate(target,value);owner.motion.flags|=0x80;}
        else {owner.motion.flags|=4;owner.motion.position=value;source_device_group_set(target,value);collision_dirty_=true;}
        return true;
    }
    return source_device_group_set(target,value);
}
float SourceDeviceWorld::script_group_value(std::uint32_t id)const {
    if(id==null_datum)return 0;return groups_.at(id&0xffffu).value;
}
bool SourceDeviceWorld::script_group_set(std::uint32_t id,float value,bool immediate) {
    if(replicated())throw FormatError("Client cannot script device groups");
    if(!std::isfinite(value))throw FormatError("Nonfinite scripted device group");
    if(id==null_datum)return false;auto &target=groups_.at(id&0xffffu);
    if(immediate){script_immediate(target,value);return true;}return source_device_group_set(target,value);
}
}
