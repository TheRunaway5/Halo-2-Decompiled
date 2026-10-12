#include "halo2/source_scene_physics.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/scene.hpp"
#include "halo2/source_shadow.hpp"
#include <bit>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t p,unsigned n=4) {
    std::uint32_t value=0;const auto bytes=memory.view(p,n);
    for(unsigned i=0;i<n;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
float scalar(const SourceRuntimeMemory &memory,std::uint32_t p) {
    return std::bit_cast<float>(word(memory,p));
}
Vec3 vector(const SourceRuntimeMemory &memory,std::uint32_t p) {
    return {scalar(memory,p),scalar(memory,p+4u),scalar(memory,p+8u)};
}
std::uint32_t body(const SourceRuntimeMemory &memory,std::uint32_t pool,std::uint32_t object,unsigned minimum=0xa4u) {
    const SourceRawDatumPoolServices storage{
        .read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto header=source_raw_datum_lookup(pool,object,storage);
    if(!header || word(memory,header+6u,2)<minimum)
        throw FormatError("Source scene transform requires a live full object with root storage");
    const auto result=word(memory,header+8u);
    if(!result)throw FormatError("Source scene transform has no retained object body");
    memory.view(result,word(memory,header+6u,2));
    return result;
}
Transform matrix(const SourceRuntimeMemory &memory,std::uint32_t p) {
    memory.view(p,0x34);
    return {scalar(memory,p),{vector(memory,p+4),vector(memory,p+16),vector(memory,p+28)},vector(memory,p+40)};
}
}
Transform source_scene_physics_transform(const SourceRuntimeMemory &memory,
    std::uint32_t pool,std::uint32_t object) {
    const auto current=body(memory,pool,object),parent=word(memory,current+0x14u);
    Transform parent_node;const Transform *attached=nullptr;bool parent_mirrored=false;
    if(parent!=0xffffffffu) {
        const auto owner=body(memory,pool,parent,0x118u);
        const auto node=std::bit_cast<std::int8_t>(std::uint8_t(word(memory,current+0x18u,1)));
        const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(memory,owner+0x114u,2)));
        const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(memory,owner+0x116u,2)));
        const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
        const auto header=source_raw_datum_lookup(pool,parent,storage);
        if(node<0 || bytes<=0 || bytes%0x34 || offset<0 || node>=bytes/0x34 ||
            unsigned(offset)+unsigned(bytes)>word(memory,header+6u,2))
            throw FormatError("Source attached scene transform has no current parent WORLD node");
        parent_node=matrix(memory,owner+std::uint32_t(offset)+std::uint32_t(node)*0x34u);
        attached=&parent_node;parent_mirrored=(word(memory,owner+4u)&0x400u)!=0;
    }
    return source_object_transform(vector(memory,current+0x64u),vector(memory,current+0x70u),
        vector(memory,current+0x7cu),scalar(memory,current+0xa0u),
        (word(memory,current+4u)&0x400u)!=0,attached,parent_mirrored);
}
std::vector<Transform> source_scene_physics_nodes(const SourceRuntimeMemory &memory,
    std::uint32_t pool,std::uint32_t object) {
    const auto current=body(memory,pool,object,0x118u);
    const auto bytes=std::bit_cast<std::int16_t>(std::uint16_t(word(memory,current+0x114u,2)));
    const auto offset=std::bit_cast<std::int16_t>(std::uint16_t(word(memory,current+0x116u,2)));
    if(!bytes)return {};
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return memory.view(p,n);}};
    const auto header=source_raw_datum_lookup(pool,object,storage);
    if(bytes<0 || bytes%0x34 || offset<0 || unsigned(offset)+unsigned(bytes)>word(memory,header+6u,2))
        throw FormatError("Source scene object has no complete current WORLD hierarchy");
    std::vector<Transform> result;result.reserve(unsigned(bytes)/0x34u);
    for(unsigned n=0;n<unsigned(bytes);n+=0x34u)result.push_back(matrix(memory,current+unsigned(offset)+n));
    return result;
}
void publish_source_scene_physics(SceneObjects &scene,const SourceRuntimeMemory &memory,
    std::uint32_t pool) {
    // Resolve the complete batch before publication: an invalid source owner
    // cannot leave only the first few scene objects updated for this tick.
    struct Solved {std::uint32_t handle;Transform root;std::vector<Transform> nodes;};
    std::vector<Solved> solved;
    for(const auto &object:scene.objects())if(object.handle!=0xffffffffu)
        solved.push_back({object.handle,source_scene_physics_transform(memory,pool,object.handle),
            source_scene_physics_nodes(memory,pool,object.handle)});
    for(auto &[handle,transform,nodes]:solved) {
        auto &object=scene.get(handle);object.solved_transform=transform;
        object.solved_nodes=std::move(nodes);
        object.position=transform.position;object.scale=transform.scale;
    }
}
}
