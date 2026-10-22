#include "halo2/source_object_location.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <functional>

namespace halo2 {
namespace {
float distance(const ShaderVector &p,Vec3 point) {
    return ((p[2]*point.z+p[1]*point.y)+p[0]*point.x)-p[3];
}
SourceObjectLocation leaf_location(const BspAtmosphere &bank,std::uint32_t leaf,std::int16_t bsp) {
    if(leaf==0xffffffffu)return {leaf,-1,bsp};
    if(leaf>=bank.leaves.size())throw FormatError("source object location leaf outside authored BSP");
    return {leaf,std::int16_t(bank.leaves[leaf]),bsp};
}
}
SourceObjectLocation source_object_point_location(const BspAtmosphere &bank,
    std::int16_t bsp,Vec3 point) {
    if(bsp==-1)return {};
    if(bsp<0 || bank.nodes.empty())throw FormatError("source object location has no active collision bank");
    std::uint32_t node=0;
    for(std::size_t visited=0;visited<=bank.nodes.size();++visited) {
        if(node&0x800000u)return leaf_location(bank,node==0xffffffu?0xffffffffu:node&0x7fffffu,bsp);
        if(node>=bank.nodes.size())throw FormatError("source object location node outside authored BSP");
        const auto &n=bank.nodes[node];
        if(n.plane>=bank.planes.size())throw FormatError("source object location plane outside authored BSP");
        node=distance(bank.planes[n.plane],point)>=0?n.positive:n.negative;
    }
    throw FormatError("source object location collision tree contains a cycle");
}
std::vector<std::uint32_t> source_object_sphere_leaves(const BspAtmosphere &bank,
    ObjectLightSpatialSphere sphere) {
    if(bank.nodes.empty())throw FormatError("source sphere leaf query has no active collision bank");
    std::vector<std::uint32_t> leaves;std::vector<bool> path(bank.nodes.size());
    std::function<void(std::uint32_t,unsigned)> visit=[&](std::uint32_t node,unsigned straddles) {
        if(node&0x800000u) {
            if(node!=0xffffffu && leaves.size()<256) {
                const auto leaf=node&0x7fffffu;
                if(leaf>=bank.leaves.size())throw FormatError("source sphere leaf outside authored BSP");
                leaves.push_back(leaf);
            }
            return;
        }
        if(node>=bank.nodes.size() || path[node])
            throw FormatError("source sphere leaf tree exceeds original traversal scratch");
        path[node]=true;const auto &n=bank.nodes[node];
        if(n.plane>=bank.planes.size())throw FormatError("source sphere leaf plane outside authored BSP");
        const auto side=distance(bank.planes[n.plane],sphere.center);
        if(side<=0.f-sphere.radius)visit(n.negative,straddles);
        else if(side<sphere.radius){
            if(straddles>=128)throw FormatError("source sphere leaf query exceeds original plane scratch");
            visit(n.negative,straddles+1);visit(n.positive,straddles+1);
        }
        else visit(n.positive,straddles);
        path[node]=false;
    };
    visit(0,0);return leaves;
}
SourceObjectLocation source_object_initial_location(const BspAtmosphere &bank,
    std::int16_t bsp,ObjectLightSpatialSphere primitive,Vec3 root) {
    auto location=source_object_point_location(bank,bsp,primitive.center);
    if(location.cluster!=-1)return location;
    //BF510 does invoke the sphere service even when the selected BSP isNONE;
    // a missing service/bank cannot be replaced by a guessed location.
    if(bsp==-1)throw FormatError("source initial location requires current collision sphere service");
    const auto leaves=source_object_sphere_leaves(bank,primitive);
    if(!leaves.empty())return leaf_location(bank,leaves.front(),bsp);
    return source_object_point_location(bank,bsp,root);
}
SourceObjectLocation source_object_default_registration_location(const BspAtmosphere &bank,
    std::int16_t bsp,Vec3 primitive,Vec3 root) {
    const auto location=source_object_point_location(bank,bsp,primitive);
    return location.cluster!=-1?location:source_object_point_location(bank,bsp,root);
}
}
