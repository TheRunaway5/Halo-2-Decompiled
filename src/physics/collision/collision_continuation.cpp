#include "halo2/collision.hpp"
#include <algorithm>
#include <cmath>

namespace halo2 {
CollisionWorld::AuthorityCheckpoint CollisionWorld::authority_checkpoint()const {
    AuthorityCheckpoint saved;saved.triangles.assign(triangles().begin(),triangles().end());
    saved.order=acceleration_->order;saved.nodes=acceleration_->nodes;return saved;
}
std::shared_ptr<const CollisionWorld> CollisionWorld::restore_authority(const AuthorityCheckpoint &saved) {
    const auto count=saved.triangles.size();
    if(saved.version!=1||count>UINT32_MAX||saved.order.size()!=count||
        (count==0?!saved.nodes.empty():saved.nodes.empty())||saved.nodes.size()>2*std::uint64_t(count))
        throw FormatError("Collision continuation has invalid native BVH shape");
    const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    for(const auto &t:saved.triangles)if(!finite(t.a)||!finite(t.b)||!finite(t.c)||!finite(t.source_normal))
        throw FormatError("Collision continuation has nonfinite triangle values");
    std::vector<bool> triangles_seen(count),nodes_seen(saved.nodes.size());
    for(auto i:saved.order){if(i>=count||triangles_seen[i])throw FormatError("Collision continuation traversal is not a permutation");triangles_seen[i]=true;}
    std::vector<std::size_t> pending;if(count)pending.push_back(0);
    const auto minimum=[](Vec3 a,Vec3 b){return Vec3{std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)};};
    const auto maximum=[](Vec3 a,Vec3 b){return Vec3{std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)};};
    const auto equal=[](Vec3 a,Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
    while(!pending.empty()){
        const auto i=pending.back();pending.pop_back();
        if(i>=saved.nodes.size()||nodes_seen[i])throw FormatError("Collision continuation has cyclic/aliased BVH nodes");
        nodes_seen[i]=true;const auto &n=saved.nodes[i];
        if(n.begin>=n.end||n.end>count||(i==0&&(n.begin!=0||n.end!=count)))
            throw FormatError("Collision continuation BVH ranges differ from their geometry");
        Vec3 low{INFINITY,INFINITY,INFINITY},high{-INFINITY,-INFINITY,-INFINITY};
        for(auto at=n.begin;at<n.end;++at){const auto &t=saved.triangles[saved.order[at]];
            low=minimum(low,minimum(t.a,minimum(t.b,t.c)));high=maximum(high,maximum(t.a,maximum(t.b,t.c)));}
        // Numeric equality admits both zero signs while retaining the saved
        // original bits. Parent bounds were built before nth_element changed
        // the final permutation, so tie-zero operand order may differ.
        if(!equal(low,n.low)||!equal(high,n.high))throw FormatError("Collision continuation BVH bounds omit/alter geometry");
        if(n.end-n.begin<=8){if(n.left!=-1||n.right!=-1)throw FormatError("Collision continuation leaf has children");}
        else {
            if(n.left<0||n.right<0||std::size_t(n.left)>=saved.nodes.size()||std::size_t(n.right)>=saved.nodes.size())
                throw FormatError("Collision continuation internal node has invalid children");
            const auto &a=saved.nodes[unsigned(n.left)],&b=saved.nodes[unsigned(n.right)];const auto mid=(std::uint64_t(n.begin)+n.end)/2;
            if(a.begin!=n.begin||a.end!=mid||b.begin!=mid||b.end!=n.end)
                throw FormatError("Collision continuation children do not partition their native range");
            pending.push_back(unsigned(n.left));pending.push_back(unsigned(n.right));
        }
    }
    if(std::find(nodes_seen.begin(),nodes_seen.end(),false)!=nodes_seen.end())throw FormatError("Collision continuation has orphaned BVH nodes");
    auto geometry=std::make_shared<std::vector<CollisionTriangle>>(saved.triangles);auto acceleration=std::make_shared<Acceleration>();
    acceleration->order=saved.order;acceleration->nodes=saved.nodes;
    return std::shared_ptr<const CollisionWorld>(new CollisionWorld(std::move(geometry),count,std::move(acceleration)));
}
}
