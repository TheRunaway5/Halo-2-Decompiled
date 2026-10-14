#include "halo2/static_scenery_collision.hpp"
#include "world_continuation_bindings.hpp"
#include <algorithm>

namespace halo2 {
namespace {
using namespace world_continuation;
std::vector<CollisionTriangle> validate(const StaticSceneryCollision::AuthorityCheckpoint &saved,
    const LevelContent &content,const SceneObjects::AuthorityCheckpoint &scene) {
    if(saved.version!=1 || scene.version!=1 || saved.scene_binding!=scene.content_binding || saved.owners.size()>2048 ||
        saved.definition_binding!=scenery_definitions(content))
        throw FormatError("Scenery collision continuation has incompatible scene or definition bindings");
    std::vector<CollisionTriangle> rebuilt;
    for(const auto &[handle,o]:saved.owners) {
        scene_owner(scene,handle,o.placement);
        if(o.placement>=content.static_scenery_collision.size() || !content.static_scenery_collision[o.placement] ||
            !finite(o.root) || (o.solved_nodes && !std::ranges::all_of(*o.solved_nodes,[](const auto &n){return finite(n);})))
            throw FormatError("Scenery collision continuation has an invalid authored pose");
        if(!o.active)continue;
        const auto &d=*content.static_scenery_collision[o.placement];
        const auto nodes=o.solved_nodes?*o.solved_nodes:pose_model_nodes_world(d.hierarchy,o.root);
        if(nodes.size()!=d.hierarchy.bones.size())throw FormatError("Scenery continuation lost its current WORLD hierarchy");
        auto part=model_collision_triangles(d.collision,nodes,d.permutations,o.placement);
        rebuilt.insert(rebuilt.end(),part.begin(),part.end());
    }
    for(const auto &t:saved.triangles)if(!finite(t.a) || !finite(t.b) || !finite(t.c) || !finite(t.source_normal))
        throw FormatError("Scenery collision continuation has a nonfinite cached triangle");
    if(triangles(rebuilt)!=triangles(saved.triangles))
        throw FormatError("Scenery collision continuation cached values/order differ from its posed definitions");
    return rebuilt;
}
}
struct StaticSceneryCollision::PreparedAuthority::Impl {
    StaticSceneryCollision *owner{};bool *staging{};
    std::map<std::uint32_t,Owner> owners;
    std::vector<CollisionTriangle> triangles;
    ~Impl(){if(staging)*staging=false;}
};
StaticSceneryCollision::PreparedAuthority::PreparedAuthority()=default;
StaticSceneryCollision::PreparedAuthority::~PreparedAuthority()=default;
StaticSceneryCollision::PreparedAuthority::PreparedAuthority(PreparedAuthority &&) noexcept=default;
StaticSceneryCollision::PreparedAuthority &StaticSceneryCollision::PreparedAuthority::operator=(PreparedAuthority &&) noexcept=default;
bool StaticSceneryCollision::PreparedAuthority::for_owner(const StaticSceneryCollision &owner)const noexcept{return impl_ && impl_->owner==&owner;}
StaticSceneryCollision::AuthorityCheckpoint StaticSceneryCollision::authority_checkpoint(const SceneObjects::AuthorityCheckpoint &scene)const {
    authority_.capture();AuthorityCheckpoint saved;saved.scene_binding=scene.content_binding;
    saved.definition_binding=scenery_definitions(*content_);saved.owners=owners_;saved.triangles=triangles_;
    (void)validate(saved,*content_,scene);return saved;
}
StaticSceneryCollision::PreparedAuthority StaticSceneryCollision::prepare_authority(const AuthorityCheckpoint &saved,
    const SceneObjects::AuthorityCheckpoint &scene) {
    authority_.prepare();PreparedAuthority result;result.impl_=std::make_unique<PreparedAuthority::Impl>();
    result.impl_->owner=this;result.impl_->staging=&authority_.staging;authority_.staging=true;
    (void)validate(saved,*content_,scene);result.impl_->owners=saved.owners;result.impl_->triangles=saved.triangles;return result;
}
bool StaticSceneryCollision::commit_authority(PreparedAuthority &&prepared) noexcept {
    if(!prepared.for_owner(*this) || !authority_.staging || authority_.depth)return false;
    owners_.swap(prepared.impl_->owners);triangles_.swap(prepared.impl_->triangles);
    authority_.fault=false;prepared.impl_.reset();return true;
}
}
