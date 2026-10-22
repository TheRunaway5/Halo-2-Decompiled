#include "halo2/source_device_world.hpp"
#include "halo2/static_scenery_collision.hpp"
#include "runtime/world_continuation_bindings.hpp"
#include <algorithm>
#include <bit>
#include <set>

namespace halo2 {
namespace {
using Checkpoint=SourceDeviceWorld::AuthorityCheckpoint;
bool same(float a,float b){return std::bit_cast<std::uint32_t>(a)==std::bit_cast<std::uint32_t>(b);}
bool same(Vec3 a,Vec3 b){return same(a.x,b.x)&&same(a.y,b.y)&&same(a.z,b.z);}
bool same(const Transform &a,const Transform &b){
    if(!same(a.scale,b.scale)||!same(a.position,b.position))return false;
    for(unsigned i=0;i<3;++i)if(!same(a.basis[i],b.basis[i]))return false;
    return true;
}
void validate(const Checkpoint &saved,const LevelContent &content,std::uint64_t completed_tick) {
    using namespace world_continuation;
    if(saved.version!=2 || saved.state.tick!=completed_tick || saved.collision_poses.size()>2048 ||
        saved.definition_binding!=device_definitions(content))
        throw FormatError("Device authority checkpoint version/tick/definition binding differs");
    validate_source_device_world_snapshot(saved.state);
    for(const auto &event:saved.pending_events)
        if(event.handle==null_datum || !(event.handle>>16) || (event.handle&65535u)>=2048 ||
            event.tag==null_datum || event.serial || event.tick)
            throw FormatError("Invalid pending device authority effect request");
    std::set<std::uint32_t> slots;std::set<std::size_t> placements;
    std::vector<CollisionTriangle> rebuilt;
    for(const auto &[handle,p]:saved.collision_poses) {
        if(handle!=p.handle || handle==null_datum || !(handle>>16) || (handle&65535u)>=2048 ||
            !slots.insert(handle&65535u).second || !placements.insert(p.placement).second ||
            p.placement>=content.devices.size() || !content.devices[p.placement] ||
            p.created_tick>completed_tick || !finite(p.root) || p.root.scale<=0 ||
            !std::isfinite(p.position) || !std::isfinite(p.power))
            throw FormatError("Device continuation has invalid cached collision pose/identity");
        const auto &d=*content.devices[p.placement];
        if(!p.bsp_hidden && d.collision) {
            if(!d.machine)throw FormatError("Device cached collision lacks its actual machine channels");
            const auto locals=source_machine_local_pose(d.collision->hierarchy,*d.machine,p.position,p.power);
            const auto nodes=pose_model_locals_nodes_world(d.collision->hierarchy,p.root,locals);
            auto part=model_collision_triangles(d.collision->collision,nodes,d.collision->permutations,p.placement);
            rebuilt.insert(rebuilt.end(),part.begin(),part.end());
        }
    }
    for(const auto &t:saved.triangles)if(!finite(t.a)||!finite(t.b)||!finite(t.c)||!finite(t.source_normal))
        throw FormatError("Device continuation has a nonfinite cached triangle");
    if(triangles(rebuilt)!=triangles(saved.triangles))
        throw FormatError("Device continuation cached collision/order differs from its retained poses");
    if(!saved.collision_dirty) {
        if(saved.collision_poses.size()!=saved.state.owners.size())
            throw FormatError("Clean device collision has different current owners");
        for(const auto &entry:saved.state.owners) {
            const auto &o=entry.state;const auto at=saved.collision_poses.find(o.handle);
            if(at==saved.collision_poses.end())throw FormatError("Clean device collision lost a current generation");
            const auto &p=at->second;
            if(p.placement!=o.placement || p.created_tick!=o.created_tick || !same(p.root,o.root) ||
                !same(p.position,o.motion.position) || !same(p.power,o.motion.power) || p.bsp_hidden!=o.bsp_hidden)
                throw FormatError("Clean device collision differs from its current pose");
        }
    }
}
}
struct SourceDeviceWorld::PreparedAuthority::Impl {
    SourceDeviceWorld *owner{};bool *staging{};
    std::unique_ptr<SourceDeviceWorld> next;
    ~Impl(){if(staging)*staging=false;}
};
SourceDeviceWorld::PreparedAuthority::PreparedAuthority()=default;
SourceDeviceWorld::PreparedAuthority::~PreparedAuthority()=default;
SourceDeviceWorld::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
SourceDeviceWorld::PreparedAuthority &SourceDeviceWorld::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool SourceDeviceWorld::PreparedAuthority::for_owner(const SourceDeviceWorld &owner)const noexcept {
    return impl_ && impl_->owner==&owner;
}
SourceDeviceWorld::AuthorityCheckpoint SourceDeviceWorld::authority_checkpoint(std::uint64_t tick)const {
    authority_.capture();
    if(replicated())throw FormatError("A device display replica is not an authority checkpoint");
    AuthorityCheckpoint saved;saved.state=snapshot(tick);saved.pending_events=events_;
    saved.definition_binding=world_continuation::device_definitions(*content_);
    saved.collision_poses=collision_poses_;saved.triangles=triangles_;saved.collision_dirty=collision_dirty_;
    validate(saved,*content_,tick);return saved;
}
SourceDeviceWorld::PreparedAuthority SourceDeviceWorld::prepare_authority(const AuthorityCheckpoint &saved,std::uint64_t completed_tick) {
    authority_.prepare();PreparedAuthority p;p.impl_=std::make_unique<PreparedAuthority::Impl>();
    p.impl_->owner=this;p.impl_->staging=&authority_.staging;authority_.staging=true;
    validate(saved,*content_,completed_tick);
    auto &next=p.impl_->next;next=std::make_unique<SourceDeviceWorld>(*content_);
    // Existing authored placement/group admission remains shared with display
    // snapshots, but the complete authority input supplies all additional banks.
    next->apply_snapshot(saved.state);next->replica_tick_.reset();next->events_=saved.pending_events;
    next->collision_poses_=saved.collision_poses;next->triangles_=saved.triangles;
    next->collision_dirty_=saved.collision_dirty;return p;
}
bool SourceDeviceWorld::commit_authority(PreparedAuthority &&p) noexcept {
    if(!p.for_owner(*this) || !authority_.staging || authority_.depth)return false;
    auto &n=*p.impl_->next;groups_.swap(n.groups_);owners_.swap(n.owners_);events_.swap(n.events_);
    triangles_.swap(n.triangles_);collision_poses_.swap(n.collision_poses_);collision_dirty_=n.collision_dirty_;
    replica_tick_.reset();authority_.fault=false;p.impl_.reset();return true;
}
void SourceDeviceWorld::restore_authority(const AuthorityCheckpoint &saved,std::uint64_t completed_tick) {
    auto prepared=prepare_authority(saved,completed_tick);
    if(!commit_authority(std::move(prepared)))throw FormatError("Device authority publication lost its completed owner");
}
}
