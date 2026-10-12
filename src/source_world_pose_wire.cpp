#include "halo2/source_world_pose_wire.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>

namespace halo2 {
namespace {
constexpr unsigned maximum_bodies=16;
// WORLD descriptor+114 is a signed byte size and each matrix occupies34h.
constexpr unsigned maximum_nodes=0x7fff/0x34;
void validate(bool authoritative,std::span<const SourceWorldPose> poses,
    std::span<const SourceWorldPoseOwner> owners) {
    if(poses.size()>maximum_bodies || (!authoritative && !poses.empty()))
        throw FormatError("Invalid native source WORLD authority or body count");
    std::set<std::uint32_t> objects;
    const auto finite=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);};
    for(const auto &pose:poses) {
        if(pose.object==0xffffffffu || !(pose.object>>16) || (pose.object&0xffffu)>=2048 ||
            !objects.insert(pose.object).second ||
            std::none_of(owners.begin(),owners.end(),[&](const auto &o){return o.object==pose.object;}) ||
            pose.nodes.empty() || pose.nodes.size()>maximum_nodes ||
            !finite(pose.sphere.center) || !std::isfinite(pose.sphere.radius) || pose.sphere.radius<0)
            throw FormatError("Invalid native source WORLD body identity, hierarchy or sphere");
        for(const auto &node:pose.nodes)if(!std::isfinite(node.scale) || !finite(node.position) ||
            !std::all_of(node.basis.begin(),node.basis.end(),finite))
            throw FormatError("Non-finite native source WORLD node");
    }
    if(authoritative)for(const auto &owner:owners)
        if(owner.required && owner.object!=0xffffffffu && !objects.contains(owner.object))
            throw FormatError("Authoritative source WORLD bank omitted a living roster body");
}
}
std::vector<std::byte> encode_source_world_poses(bool authoritative,
    std::span<const SourceWorldPose> poses,std::span<const SourceWorldPoseOwner> owners) {
    validate(authoritative,poses,owners);std::vector<std::byte> result;
    const auto put=[&](std::uint32_t v,unsigned n){for(unsigned i=0;i<n;++i)result.push_back(std::byte(v>>(8*i)));};
    const auto scalar=[&](float f){put(std::bit_cast<std::uint32_t>(f),4);};
    const auto vector=[&](Vec3 v){scalar(v.x);scalar(v.y);scalar(v.z);};
    put(authoritative,1);put(unsigned(poses.size()),1);
    for(const auto &pose:poses) {
        put(pose.object,4);put(unsigned(pose.nodes.size()),2);
        vector(pose.sphere.center);scalar(pose.sphere.radius);
        for(const auto &node:pose.nodes) {
            scalar(node.scale);for(auto basis:node.basis)vector(basis);vector(node.position);
        }
    }
    return result;
}
SourceWorldPoseBank decode_source_world_poses(std::span<const std::byte> bytes,
    std::span<const SourceWorldPoseOwner> owners) {
    SourceWorldPoseBank bank;
    const auto get=[&](unsigned n) {
        if(bank.consumed+n>bytes.size())throw FormatError("Truncated native source WORLD pose bank");
        std::uint32_t value=0;for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[bank.consumed++])<<(8*i);
        return value;
    };
    const auto scalar=[&]{return std::bit_cast<float>(get(4));};
    const auto vector=[&]{return Vec3{scalar(),scalar(),scalar()};};
    const auto authoritative=get(1),count=get(1);
    if(authoritative>1 || count>maximum_bodies)throw FormatError("Invalid native source WORLD pose header");
    bank.authoritative=authoritative;
    for(unsigned n=0;n<count;++n) {
        SourceWorldPose pose;pose.object=get(4);const auto nodes=get(2);
        if(!nodes || nodes>maximum_nodes || nodes*0x34u+16u>bytes.size()-bank.consumed)
            throw FormatError("Invalid native source WORLD hierarchy extent");
        pose.sphere={vector(),scalar()};pose.nodes.resize(nodes);
        for(auto &node:pose.nodes) {
            node.scale=scalar();for(auto &basis:node.basis)basis=vector();node.position=vector();
        }
        bank.poses.push_back(std::move(pose));
    }
    validate(bank.authoritative,bank.poses,owners);return bank;
}
}
