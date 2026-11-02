#include "halo2/native_tu15_camera_query_object_context.hpp"
#include <bit>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{std::array<std::byte,4>b{};m_.read(p,std::span(b).first(width));std::uint32_t n{};for(unsigned k=0;k<width;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;}
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);e.written.assign(b.begin(),b.end());}m_.write(p,b);if(o_)o_(e);}
};
std::uint32_t signed_word(std::uint32_t n){return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(n))));}
struct MemoryOutput {Memory&m;std::uint32_t identity;void word(std::uint32_t pc,unsigned offset,std::uint32_t n){m.word(pc,identity+offset,n);}};
struct HostOutput {
    NativeTu15CameraCollisionContext&value;
    void word(std::uint32_t,unsigned offset,std::uint32_t n){
        switch(offset){case 0:value.object=n;break;case 4:value.model=n;break;case 8:value.collision=n;break;case 12:value.region_state=n;break;case 16:value.node_matrices=n;break;}
    }
};
template<class Output>bool resolve(Memory&m,std::uint32_t object,Output output){
    const auto objects=m.word(0x4ecbd0u),rows=m.word(objects+0x44u),body=m.word(rows+(object&0xffffu)*12u+8u);
    const auto definition=m.word(body),tags=m.word(0x4f0414u),object_tag=m.word(tags+(definition&0xffffu)*16u+8u);
    const auto model_datum=m.word(object_tag+0x38u);if(model_datum==0xffffffffu)return false;
    const auto model=m.word(tags+(model_datum&0xffffu)*16u+8u),collision_datum=m.word(model+12u);
    if(collision_datum==0xffffffffu)return false;
    const auto region_offset=signed_word(m.word(body+0x11au,2));
    const auto collision=m.word(tags+(collision_datum&0xffffu)*16u+8u),node_offset=signed_word(m.word(body+0x116u,2));
    output.word(0x1f7e70u,12u,body+region_offset);output.word(0x1f7e73u,0u,object);
    output.word(0x1f7e75u,4u,model);output.word(0x1f7e78u,8u,collision);
    output.word(0x1f7e7bu,16u,body+node_offset);return true;
}
std::uint32_t region(Memory&m,std::uint32_t collision,std::uint32_t encoded){
    const auto sections=m.word(collision+0x20u),section=m.word(sections+(encoded>>24u)*12u+8u);
    const auto regions=m.word(section+((encoded>>16u)&0xffu)*20u+8u);
    return regions+((encoded>>8u)&0xffu)*0x44u+4u;
}
}
bool native_tu15_resolve_camera_collision_context(SourcePhysicsContactMemory&memory,
    std::uint32_t object,std::uint32_t output,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);return resolve(m,object,MemoryOutput{m,output});
}
std::optional<NativeTu15CameraCollisionContext> native_tu15_camera_collision_context(
    SourcePhysicsContactMemory&memory,std::uint32_t object){
    const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);NativeTu15CameraCollisionContext value;
    if(!resolve(m,object,HostOutput{value}))return std::nullopt;return value;
}
std::uint32_t native_tu15_camera_collision_region(SourcePhysicsContactMemory&memory,
    std::uint32_t context,std::uint32_t encoded){
    const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);return region(m,m.word(context+8u),encoded);
}
std::uint32_t native_tu15_camera_collision_region_value(SourcePhysicsContactMemory&memory,
    const NativeTu15CameraCollisionContext&context,std::uint32_t encoded){
    const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);return region(m,context.collision,encoded);
}
std::uint32_t native_tu15_camera_query_collision_geometry(SourcePhysicsContactMemory&memory,std::uint32_t packet){
    const SourcePhysicsContactRecordObserver empty;Memory m(memory,empty);const auto kind=m.word(packet);
    if(kind==1u)return m.word(0x4ecc10u);
    if(kind==3u){
        const auto index=m.word(packet+0x3cu),scenario=m.word(0x4ecc18u),instances=m.word(scenario+0x144u);
        const auto definition=signed_word(m.word(instances+index*0x58u+0x34u,2)),definitions=m.word(scenario+0x13cu);
        return definitions+definition*0xc8u+0x70u;
    }
    if(kind==4u){
        NativeTu15CameraCollisionContext context;
        if(!resolve(m,m.word(packet+0x40u),HostOutput{context}))return 0;
        const auto encoded=m.word(packet+0x48u);return region(m,context.collision,encoded);
    }
    return 0;
}
} // namespace halo2
