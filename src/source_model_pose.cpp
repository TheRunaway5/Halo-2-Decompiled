#include "halo2/source_model_pose.hpp"
#include <bit>
#include <cmath>

namespace halo2 {
namespace {
constexpr float norm_threshold=std::bit_cast<float>(0x38d1b717u);
void validate(const BonePose &pose) {
    for(const auto value:pose.rotation)
        if(!std::isfinite(value))throw FormatError("nonfinite source local quaternion");
    if(!std::isfinite(pose.translation.x) || !std::isfinite(pose.translation.y) ||
       !std::isfinite(pose.translation.z) || !std::isfinite(pose.scale))
        throw FormatError("nonfinite source local transform");
}
float sign_xor(float value) {
    return std::bit_cast<float>(std::bit_cast<std::uint32_t>(value)^0x80000000u);
}
} // namespace
float source_model_local_squared_norm(const BonePose &pose) {
    validate(pose);const auto &q=pose.rotation;
    const float x=q[0]*q[0],y=q[1]*q[1],z=q[2]*q[2],w=q[3]*q[3];
    return (x+z)+(y+w);
}
Transform source_model_local_transform(const BonePose &pose,float reciprocal_seed) {
    const float norm=source_model_local_squared_norm(pose);
    if(!std::isfinite(norm))throw FormatError("source local quaternion norm overflow");
    Transform result;result.position=pose.translation;result.scale=pose.scale;
    if(norm<=norm_threshold)return result;
    if(!std::isfinite(reciprocal_seed) || reciprocal_seed<=0)
        throw FormatError("invalid source local reciprocal seed");
    //14226F..14228B: two separately rounded products, one Newton step, then
    //double the refined inverse norm. This is not quaternion normalization.
    const float first=norm*reciprocal_seed,second=first*reciprocal_seed;
    const float twice=reciprocal_seed+reciprocal_seed;
    const float refined=twice-second,factor=refined+refined;
    const auto &q=pose.rotation;
    const float sx=q[0]*factor,sy=q[1]*factor,sz=q[2]*factor;
    const float xx=q[0]*sx,xy=q[0]*sy,xz=q[0]*sz;
    const float wx=q[3]*sx,wy=q[3]*sy,wz=q[3]*sz;
    const float yy=q[1]*sy,yz=q[1]*sz,zz=sz*q[2];
    //1422B9..14232D: sign-bit XORs and literal subtraction grouping preserve
    //signed zero and the diagonal rounding that GLM's formula changes.
    const float one_minus_zz=1.f-zz;
    result.basis={{{one_minus_zz-yy,xy+wz,xz+sign_xor(wy)},
                   {xy+sign_xor(wz),one_minus_zz-xx,yz+wx},
                   {xz+wy,yz-wx,(1.f-xx)-yy}}};
    return result;
}
Transform source_model_local_transform(const BonePose &pose) {
    const float norm=source_model_local_squared_norm(pose);
    return source_model_local_transform(pose,norm>norm_threshold?1.f/norm:1.f);
}
std::vector<Transform> source_model_world_nodes(
    std::span<const Transform> locals,std::span<const int> parents,
    const Transform *root) {
    if(locals.size()!=parents.size() || locals.size()>256)
        throw FormatError("invalid source model hierarchy bank");
    std::vector<Transform> result;result.reserve(locals.size());
    for(std::size_t i=0;i<locals.size();++i) {
        const auto parent=parents[i];
        if(parent< -1 || (parent>=0 && std::size_t(parent)>=i))
            throw FormatError("source model parent does not precede its child");
        result.push_back(parent>=0?compose_transform(result[std::size_t(parent)],locals[i]):
            root?compose_transform(*root,locals[i]):locals[i]);
    }
    return result;
}
std::vector<Transform> source_model_world_nodes(
    std::span<const BonePose> locals,std::span<const int> parents,
    const Transform *root) {
    std::vector<Transform> transforms;transforms.reserve(locals.size());
    for(const auto &local:locals)transforms.push_back(source_model_local_transform(local));
    return source_model_world_nodes(transforms,parents,root);
}
} // namespace halo2
