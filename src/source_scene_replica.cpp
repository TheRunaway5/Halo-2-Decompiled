#include "halo2/scene.hpp"
#include <algorithm>
#include <cmath>
#include <set>

namespace halo2 {
void SceneObjects::adopt_source_replicas(std::span<const SourceSceneRoot> roots,
    std::span<const SourceWorldPose> poses) {
    AuthorityCalls::Scope call(authority_calls_);
    if(source_owner_.create || lifetime_owner().pool().source_memory())
        throw FormatError("Source authority cannot become a scene presentation replica");
    if(roots.size()>2048)throw FormatError("Source replica scene exceeds the original object capacity");
    const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    const auto finite_transform=[&](const Transform &t){return std::isfinite(t.scale) &&
        finite(t.position) && std::all_of(t.basis.begin(),t.basis.end(),finite);};
    std::map<std::uint32_t,const SourceWorldPose *> world;
    for(const auto &pose:poses)if(!world.emplace(pose.object,&pose).second)
        throw FormatError("Source replica has duplicate WORLD owners");
    std::vector<Object> next;next.reserve(roots.size());
    std::map<std::uint32_t,std::size_t> slots;
    std::vector<std::uint32_t> names(names_.size(),null_datum);
    std::set<std::uint32_t> placements,indices;
    for(const auto &root:roots) {
        if(root.placement>=content_.objects.size() || root.object==null_datum || root.definition==null_datum ||
            !(root.object>>16) || (root.object&0xffffu)>=2048 ||
            !placements.insert(root.placement).second || !indices.insert(root.object&0xffffu).second ||
            content_.objects[root.placement].type==1 ||
            content_.objects[root.placement].definition!=root.definition ||
            !(root.transform.scale>0) || !finite_transform(root.transform) || !finite(root.velocity))
            throw FormatError("Source replica scene root differs from its authored placement or frame");
        const auto &authored=content_.objects[root.placement];
        const auto pose=world.find(root.object);
        if(root.has_world_nodes!=(pose!=world.end()))
            throw FormatError("Source replica scene lost its completed WORLD hierarchy");
        if(pose!=world.end() && (pose->second->nodes.empty() || pose->second->nodes.size()>0x7fff/0x34 ||
            !finite(pose->second->sphere.center) || !std::isfinite(pose->second->sphere.radius) ||
            pose->second->sphere.radius<0 ||
            !std::all_of(pose->second->nodes.begin(),pose->second->nodes.end(),finite_transform)))
            throw FormatError("Source replica scene has an invalid WORLD hierarchy or sphere");
        Object object;
        if(const auto existing=slots_.find(root.object);source_replica_ && existing!=slots_.end()) {
            const auto &prior=objects_[existing->second];
            if(prior.placement!=root.placement || prior.created_tick!=root.created_tick)
                throw FormatError("Source replica full generation changed placement or creation tick");
            object=prior;
        }else {
            object.rotation=authored.rotation;object.bsp=int(bsp_);
        }
        object.handle=root.object;object.placement=root.placement;object.created_tick=root.created_tick;
        object.solved_transform=root.transform;object.position=root.transform.position;object.scale=root.transform.scale;
        object.visible=root.visible;
        object.solved_nodes=pose==world.end()?std::vector<Transform>{}:pose->second->nodes;
        if(authored.name>=0) {
            if(std::size_t(authored.name)>=names.size() || names[authored.name]!=null_datum)
                throw FormatError("Source replica has duplicate or invalid authored names");
            names[authored.name]=root.object;
        }
        slots.emplace(root.object,next.size());next.push_back(std::move(object));
    }
    auto functions=source_replica_?functions_:decltype(functions_){};
    std::erase_if(functions,[&](const auto &entry){return !slots.contains(entry.first.first);});
    // Stage retirement of the initial local presentation in an owned pool
    // copy. A rejected bank cannot partially retire those local identities.
    // All subsequent replicas have no local lifetime/body allocations.
    if(!source_replica_) {
        auto lifetimes=lifetime_owner();
        for(const auto &object:objects_)if(object.handle!=null_datum)lifetimes.release(object.handle);
        ++authority_calls_.mutations;
        lifetime_owner()=std::move(lifetimes);
    }
    ++authority_calls_.mutations;
    objects_.swap(next);slots_.swap(slots);names_.swap(names);functions_.swap(functions);
    source_replica_=true;
}
}
