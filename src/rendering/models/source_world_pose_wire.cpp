#include "halo2/source_world_pose_wire.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <set>

namespace halo2 {
namespace {
constexpr unsigned maximum_bodies=2048; // Actual object header pool capacity.
// WORLD descriptor+114 is a signed byte size and each matrix occupies34h.
constexpr unsigned maximum_nodes=0x7fff/0x34;
void validate(bool authoritative,std::span<const SourceWorldPose> poses,
    std::span<const SourceWorldPoseOwner> owners,SourceWorldPoseWireFormat format) {
    if(format!=SourceWorldPoseWireFormat::legacy && format!=SourceWorldPoseWireFormat::parent_frame)
        throw FormatError("Unsupported native WORLD pose-bank format");
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
        if(format==SourceWorldPoseWireFormat::parent_frame) {
            if(!pose.attachment)throw FormatError("Native source WORLD pose lacks explicit attachment metadata");
            const auto &binding=*pose.attachment;
            if(binding.parent==0xffffffffu) {
                if(binding.parent_frame)throw FormatError("Unparented native WORLD pose carries a parent frame");
            } else {
                if(!(binding.parent>>16) || (binding.parent&0xffffu)>=maximum_bodies || binding.parent==pose.object ||
                    !binding.parent_frame)
                    throw FormatError("Native WORLD pose has an invalid full parent/node/frame binding");
                const auto &frame=*binding.parent_frame;
                if(!std::isfinite(frame.scale) || !finite(frame.position) ||
                    !std::all_of(frame.basis.begin(),frame.basis.end(),finite))
                    throw FormatError("Non-finite native source WORLD parent frame");
                const auto parent=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==binding.parent;});
                if(parent!=poses.end() && binding.parent_node>=parent->nodes.size())
                    throw FormatError("Native WORLD parent node exceeds its published hierarchy");
            }
        }
    }
    if(format==SourceWorldPoseWireFormat::parent_frame)for(const auto &pose:poses) {
        std::set<std::uint32_t> path{pose.object};auto next=pose.attachment->parent;
        while(next!=0xffffffffu) {
            if(!path.insert(next).second)throw FormatError("Native WORLD attachment hierarchy is cyclic");
            const auto parent=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==next;});
            if(parent==poses.end())break;
            next=parent->attachment->parent;
        }
    }
    if(authoritative)for(const auto &owner:owners)
        if(owner.required && owner.object!=0xffffffffu && !objects.contains(owner.object))
            throw FormatError("Authoritative source WORLD bank omitted a living roster body");
}
}
std::vector<std::byte> encode_source_world_poses(bool authoritative,
    std::span<const SourceWorldPose> poses,std::span<const SourceWorldPoseOwner> owners,
    SourceWorldPoseWireFormat format) {
    validate(authoritative,poses,owners,format);std::vector<std::byte> result;
    const auto put=[&](std::uint32_t v,unsigned n){for(unsigned i=0;i<n;++i)result.push_back(std::byte(v>>(8*i)));};
    const auto scalar=[&](float f){put(std::bit_cast<std::uint32_t>(f),4);};
    const auto vector=[&](Vec3 v){scalar(v.x);scalar(v.y);scalar(v.z);};
    if(format==SourceWorldPoseWireFormat::parent_frame) {
        for(const auto c:std::array{'H','2','W','P'})put(unsigned(c),1);
        put(1,1); // Explicit inner WORLD pose-bank format, independent of body version.
    }
    put(authoritative,1);put(unsigned(poses.size()),2);
    for(const auto &pose:poses) {
        put(pose.object,4);put(unsigned(pose.nodes.size()),2);
        if(format==SourceWorldPoseWireFormat::parent_frame) {
            const auto &binding=*pose.attachment;put(binding.parent,4);put(binding.parent_node,1);
            if(binding.parent_frame) {
                const auto &frame=*binding.parent_frame;scalar(frame.scale);
                for(auto basis:frame.basis)vector(basis);vector(frame.position);
            }
        }
        vector(pose.sphere.center);scalar(pose.sphere.radius);
        for(const auto &node:pose.nodes) {
            scalar(node.scale);for(auto basis:node.basis)vector(basis);vector(node.position);
        }
    }
    return result;
}
SourceWorldPoseBank decode_source_world_poses(std::span<const std::byte> bytes,
    std::span<const SourceWorldPoseOwner> owners,SourceWorldPoseWireFormat format) {
    if(format!=SourceWorldPoseWireFormat::legacy && format!=SourceWorldPoseWireFormat::parent_frame)
        throw FormatError("Unsupported native WORLD pose-bank format");
    SourceWorldPoseBank bank;
    const auto get=[&](unsigned n) {
        if(bank.consumed+n>bytes.size())throw FormatError("Truncated native source WORLD pose bank");
        std::uint32_t value=0;for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[bank.consumed++])<<(8*i);
        return value;
    };
    const auto scalar=[&]{return std::bit_cast<float>(get(4));};
    const auto vector=[&]{return Vec3{scalar(),scalar(),scalar()};};
    if(format==SourceWorldPoseWireFormat::parent_frame) {
        for(const auto c:std::array{'H','2','W','P'})
            if(get(1)!=unsigned(c))throw FormatError("Native WORLD pose-bank format differs from its snapshot version");
        if(get(1)!=1u)throw FormatError("Unsupported native WORLD pose-bank inner version");
    }
    const auto authoritative=get(1),count=get(2);
    if(authoritative>1 || count>maximum_bodies)throw FormatError("Invalid native source WORLD pose header");
    bank.authoritative=authoritative;
    for(unsigned n=0;n<count;++n) {
        SourceWorldPose pose;pose.object=get(4);const auto nodes=get(2);
        if(format==SourceWorldPoseWireFormat::parent_frame) {
            SourceWorldPoseAttachment binding;binding.parent=get(4);binding.parent_node=std::uint8_t(get(1));
            if(binding.parent!=0xffffffffu) {
                Transform frame;frame.scale=scalar();for(auto &basis:frame.basis)basis=vector();frame.position=vector();
                binding.parent_frame=frame;
            }
            pose.attachment=binding;
        }
        if(!nodes || nodes>maximum_nodes || nodes*0x34u+16u>bytes.size()-bank.consumed)
            throw FormatError("Invalid native source WORLD hierarchy extent");
        pose.sphere={vector(),scalar()};pose.nodes.resize(nodes);
        for(auto &node:pose.nodes) {
            node.scale=scalar();for(auto &basis:node.basis)basis=vector();node.position=vector();
        }
        bank.poses.push_back(std::move(pose));
    }
    validate(bank.authoritative,bank.poses,owners,format);return bank;
}
std::vector<std::byte> encode_source_world_pose_models(std::span<const SourceWorldPose> poses) {
    if(poses.size()>maximum_bodies)throw FormatError("Native WORLD model metadata exceeds object capacity");
    std::vector<std::byte> result;unsigned count{};std::set<std::uint32_t> owners;
    for(const auto &pose:poses)if(pose.model)++count;
    const auto put=[&](std::uint32_t value,unsigned bytes) {
        for(unsigned n=0;n<bytes;++n)result.push_back(std::byte(value>>(n*8u)));
    };
    put(count,2);
    for(const auto &pose:poses)if(pose.model) {
        const auto &model=*pose.model;
        if(pose.object==0xffffffffu || !(pose.object>>16) || (pose.object&65535u)>=maximum_bodies ||
            !owners.insert(pose.object).second || model.definition==0xffffffffu || model.permutations.size()>16)
            throw FormatError("Native WORLD model metadata has invalid owner/definition/region extent");
        put(pose.object,4);put(model.definition,4);put(unsigned(model.permutations.size()),1);
        for(auto permutation:model.permutations)put(permutation,1);
    }
    return result;
}
std::size_t decode_source_world_pose_models(std::span<const std::byte> bytes,std::span<SourceWorldPose> poses) {
    std::size_t consumed{};
    const auto get=[&](unsigned size) {
        if(size>bytes.size()-consumed)throw FormatError("Truncated native WORLD model metadata");
        std::uint32_t value{};
        for(unsigned n=0;n<size;++n)value|=std::to_integer<std::uint32_t>(bytes[consumed++])<<(n*8u);
        return value;
    };
    const auto count=get(2);
    if(count>maximum_bodies || count>poses.size())throw FormatError("Native WORLD model metadata exceeds its pose bank");
    std::set<std::uint32_t> owners;
    for(auto &pose:poses)pose.model.reset();
    for(unsigned n=0;n<count;++n) {
        const auto owner=get(4);SourceWorldPoseModel model;model.definition=get(4);const auto regions=get(1);
        const auto pose=std::find_if(poses.begin(),poses.end(),[&](const auto &p){return p.object==owner;});
        if(!owners.insert(owner).second || pose==poses.end() || model.definition==0xffffffffu || regions>16)
            throw FormatError("Native WORLD model metadata lost its accepted full owner/definition/region extent");
        for(unsigned region=0;region<regions;++region)model.permutations.push_back(std::uint8_t(get(1)));
        pose->model=std::move(model);
    }
    return consumed;
}
}
