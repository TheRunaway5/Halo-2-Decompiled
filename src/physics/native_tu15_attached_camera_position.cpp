#include "halo2/native_tu15_attached_camera_position.hpp"
#include "halo2/native_campaign_object_marker_request_tu15.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const{
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t n{};
        for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t n){
        std::array<std::byte,4>b{};for(unsigned k=0;k<4;++k)b[k]=std::byte(n>>(k*8u));
        SourcePhysicsContactRecordStore e{pc,p,{},{}};if(o_){e.before.resize(4);m_.read(p,e.before);}
        m_.write(p,b);if(o_){e.written.assign(b.begin(),b.end());o_(e);}
    }
};
}
void native_tu15_compute_attached_camera_position(SourcePhysicsContactMemory&memory,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp&fp,
    unsigned depth,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15AttachedCameraPositionStats*stats){
    if(stats)++stats->calls;
    try{
        if(depth)throw std::runtime_error("Native attached camera requires its qualified empty-x87 marker domain");
        Memory m(memory,observe);
        const auto objects=m.word(0x4ecbd0u),rows=m.word(objects+0x44u);
        const auto unit=m.word(rows+(object&0xffffu)*12u+8u);
        const auto definition=m.word(unit),tags=m.word(0x4f0414u);
        // The source caches this current tag even on the attached branch.
        (void)m.word(tags+(definition&0xffffu)*16u+8u);
        const auto initial_parent=m.word(unit+0x14u);
        if(initial_parent==0xffffffffu){
            if(stats)++stats->on_foot_requirements;
            throw std::runtime_error("Native camera requires its genuine on-foot collision/crouch computation");
        }
        if(stats)++stats->base_position_calls;
        (void)native_tu15_object_camera_origin(memory,initial_parent,destination,fp,observe);
        const auto seat_word=std::uint16_t(m.word(unit+0x1fcu,2));
        if(seat_word!=0xffffu){
            const auto parent=m.word(unit+0x14u),current_objects=m.word(0x4ecbd0u);
            const auto current_rows=m.word(current_objects+0x44u),current_tags=m.word(0x4f0414u);
            const auto parent_record=m.word(current_rows+(parent&0xffffu)*12u+8u);
            const auto parent_definition=m.word(parent_record);
            const auto tag=m.word(current_tags+(parent_definition&0xffffu)*16u+8u);
            const auto seats=m.word(tag+0x1ccu);
            const auto kind=m.word(parent_record+0xaau,1);
            const auto seat=seats+std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(seat_word)))*0xb0u;
            bool use_marker=true;
            if(kind==1u)use_marker=m.word(seat+0x60u)!=0;
            if(use_marker){
                const auto marker=m.word(seat+0x60u);
                if(stats)++stats->marker_requests;
                const auto result=native_campaign_object_marker_request_value_tu15(memory,parent,false,marker,fp);
                const auto x=result.value.world[10],y=result.value.world[11],z=result.value.world[12];
                m.word(0xcd8f1u,destination,x);
                m.word(0xcd8f3u,destination+4u,y);
                m.word(0xcd8f6u,destination+8u,z);
            }
        }
        if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
