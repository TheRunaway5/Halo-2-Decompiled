#include "halo2/source_scene_physics.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_shadow.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> bytes) {
    std::uint32_t value=0;
    for(unsigned i=0;i<bytes.size();++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
float scalar(std::span<const std::byte> bytes,unsigned offset) {
    return std::bit_cast<float>(word(bytes.subspan(offset,4)));
}
Vec3 vector(std::span<const std::byte> bytes,unsigned offset) {
    return {scalar(bytes,offset),scalar(bytes,offset+4u),scalar(bytes,offset+8u)};
}
std::span<const std::byte> source_body(const SourceRuntimeMemory &memory,std::uint32_t header,unsigned minimum) {
    if(!header)
        throw FormatError("Source scene transform requires a live full object with root storage");
    const auto size=word(memory.view(header+6u,2));
    if(size<minimum)
        throw FormatError("Source scene transform requires a live full object with root storage");
    const auto address=word(memory.view(header+8u,4));
    if(!address)throw FormatError("Source scene transform has no retained object body");
    return memory.view(address,size);
}
Transform matrix(std::span<const std::byte> bytes) {
    return {scalar(bytes,0),{vector(bytes,4),vector(bytes,16),vector(bytes,28)},vector(bytes,40)};
}
Transform transform_from_body(const SourceRuntimeMemory &memory,std::uint32_t pool,
    const SourceRawDatumPoolServices &storage,std::span<const std::byte> current) {
    const auto parent=word(current.subspan(0x14u,4));
    Transform parent_node;const Transform *attached=nullptr;bool parent_mirrored=false;
    if(parent!=0xffffffffu) {
        const auto owner=source_body(memory,source_raw_datum_lookup(pool,parent,storage),0x118u);
        const auto node=std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(current[0x18u]));
        const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(owner.subspan(0x114u,2))));
        const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(owner.subspan(0x116u,2))));
        if(node<0 || bytes<=0 || bytes%0x34 || offset<0 || node>=bytes/0x34 ||
            unsigned(offset)+unsigned(bytes)>owner.size())
            throw FormatError("Source attached scene transform has no current parent WORLD node");
        parent_node=matrix(owner.subspan(unsigned(offset)+unsigned(node)*0x34u,0x34u));
        attached=&parent_node;parent_mirrored=(word(owner.subspan(4u,4))&0x400u)!=0;
    }
    return source_object_transform(vector(current,0x64u),vector(current,0x70u),
        vector(current,0x7cu),scalar(current,0xa0u),
        (word(current.subspan(4u,4))&0x400u)!=0,attached,parent_mirrored);
}
std::vector<Transform> nodes_from_body(std::span<const std::byte> current) {
    if(current.size()<0x118u)
        throw FormatError("Source scene transform requires a live full object with root storage");
    const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(current.subspan(0x114u,2))));
    const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(current.subspan(0x116u,2))));
    if(!bytes)return {};
    if(bytes<0 || bytes%0x34 || offset<0 || unsigned(offset)+unsigned(bytes)>current.size())
        throw FormatError("Source scene object has no complete current WORLD hierarchy");
    std::vector<Transform> result;result.reserve(unsigned(bytes)/0x34u);
    for(unsigned n=0;n<unsigned(bytes);n+=0x34u)result.push_back(matrix(current.subspan(unsigned(offset)+n,0x34u)));
    return result;
}
}
Transform source_scene_physics_transform(const SourceRuntimeMemory &memory,
    std::uint32_t pool,std::uint32_t object) {
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto body=source_body(memory,source_raw_datum_lookup(pool,object,storage),0xa4u);
    return transform_from_body(memory,pool,storage,body);
}
std::vector<Transform> source_scene_physics_nodes(const SourceRuntimeMemory &memory,
    std::uint32_t pool,std::uint32_t object) {
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    return nodes_from_body(source_body(memory,source_raw_datum_lookup(pool,object,storage),0x118u));
}
void publish_source_scene_physics(SceneObjects &scene,const SourceRuntimeMemory &memory,
    std::uint32_t pool) {
    // Resolve the complete batch before publication: an invalid source owner
    // cannot leave only the first few scene objects updated for this tick.
    struct Solved {std::uint32_t handle;Transform root;std::vector<Transform> nodes;};
    std::vector<Solved> solved;
    std::vector<std::uint32_t> retired;
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    for(const auto &object:scene.objects())if(object.handle!=0xffffffffu) {
        const auto header=source_raw_datum_lookup(pool,object.handle,storage);
        if(!header)retired.push_back(object.handle);
        else {
            const auto body=source_body(memory,header,0xa4u);
            solved.push_back({object.handle,transform_from_body(memory,pool,storage,body),nodes_from_body(body)});
        }
    }
    for(const auto handle:retired)scene.observe_source_retirement(handle);
    for(auto &[handle,transform,nodes]:solved) {
        auto &object=scene.get(handle);object.solved_transform=transform;
        object.solved_nodes=std::move(nodes);
        object.position=transform.position;object.scale=transform.scale;
    }
}
Transform source_scene_physics_transform(const SourceRuntimeMemory &memory,
    NativeRuntimeAddress pool,std::uint32_t object) {
    return source_scene_physics_transform(memory,memory.source_address(pool,0x4c),object);
}
std::vector<Transform> source_scene_physics_nodes(const SourceRuntimeMemory &memory,
    NativeRuntimeAddress pool,std::uint32_t object) {
    return source_scene_physics_nodes(memory,memory.source_address(pool,0x4c),object);
}
void publish_source_scene_physics(SceneObjects &scene,const SourceRuntimeMemory &memory,
    NativeRuntimeAddress pool) {
    publish_source_scene_physics(scene,memory,memory.source_address(pool,0x4c));
}
}
