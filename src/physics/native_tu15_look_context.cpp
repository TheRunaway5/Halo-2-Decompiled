#include "halo2/native_tu15_look_context.hpp"
#include "halo2/native_campaign_object_world_point_tu15.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;
    const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned size=4)const {
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(size));std::uint32_t n{};
        for(unsigned k=0;k<size;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
        return n;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned size=4) {
        std::array<std::byte,4>b{};for(unsigned k=0;k<size;++k)b[k]=std::byte(value>>(k*8u));
        const auto bytes=std::span(b).first(size);SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(size);m_.read(p,e.before);}m_.write(p,bytes);
        if(o_){e.written.assign(bytes.begin(),bytes.end());o_(e);}
    }
};
}
std::uint32_t native_tu15_object_camera_origin(SourcePhysicsContactMemory& memory,
    std::uint32_t object,std::uint32_t destination,NativeTu15PhysicsFrameFp&fp,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15CameraOriginStats*stats) {
    if(stats)++stats->calls;Memory m(memory,observe);
    const auto objects=m.word(0x4ecbd0u);auto current=object,last=0xffffffffu;
    if(current!=0xffffffffu) {
        const auto rows=m.word(objects+0x44u);
        do {
            last=current;
            const auto record=m.word(rows+(current&0xffffu)*12u+8u);
            current=m.word(record+0x14u);
            if(stats)++stats->parent_steps;
        }while(current!=0xffffffffu);
    }
    const auto rows=m.word(objects+0x44u);
    const auto record=m.word(rows+(last&0xffffu)*12u+8u);
    const auto actor=m.word(record+0xb4u);
    if(actor!=0xffffffffu) {
        const auto actors=m.word(0x52be5cu),actor_rows=m.word(actors+0x44u);
        const auto row=actor_rows+(actor&0xffffu)*160u;
        const auto raw_index=std::uint8_t(m.word(row+0x18u,1));
        const auto index=std::int32_t(std::bit_cast<std::int8_t>(raw_index));
        bool valid=false;
        if(index>=0)valid=index<std::bit_cast<std::int32_t>(m.word(row+0x74u));
        if(valid) {
            const auto entries=m.word(row+0x70u);
            const auto selected=m.word(entries+std::uint32_t(index)*96u+0x40u);
            const auto data=m.word(selected+0x3cu);
            const auto z=m.word(data+0x78u),y=m.word(data+0x74u),x=m.word(data+0x70u);
            m.word(0xbbfcbu,destination,x);
            m.word(0xbbfceu,destination+4u,y);
            m.word(0xbbfd3u,destination+8u,z);
            if(stats)++stats->actor_points;
            return destination;
        }
    }
    if(stats)++stats->world_point_fallbacks;
    native_campaign_object_world_point_tu15(memory,object,destination,fp,observe);
    return destination;
}
void native_tu15_build_look_context(SourcePhysicsContactMemory&memory,
    std::uint32_t index,std::uint32_t context,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15LookContextServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15LookContextStats*stats) {
    if(stats)++stats->calls;
    try {
        Memory m(memory,observe);
        const auto controls=m.word(0x4f9b54u);
        m.word(0x189fdcu,context+8u,0);
        const auto object=m.word(controls+index*0x94u+0x14u);
        m.word(0x189fecu,context,object);
        m.word(0x189feeu,context+4u,0xffffu,2);
        if(object!=0xffffffffu) {
            const auto objects=m.word(0x4ecbd0u),rows=m.word(objects+0x44u);
            const auto unit=m.word(rows+(object&0xffffu)*12u+8u);
            if(!services.camera_position)
                throw std::runtime_error("Native look context requires completeCD4A0 camera computation");
            if(stats)++stats->camera_calls;
            services.camera_position(object,context+12u,fp);
            if(std::uint16_t(m.word(unit+0x1fcu,2))!=0xffffu) {
                const auto parent=m.word(unit+0x14u);
                const auto current_objects=m.word(0x4ecbd0u);
                const auto current_rows=m.word(current_objects+0x44u);
                const auto parent_record=m.word(current_rows+(parent&0xffffu)*12u+8u);
                const auto definition=m.word(parent_record);
                const auto tags=m.word(0x4f0414u);
                const auto tag=m.word(tags+(definition&0xffffu)*16u+8u);
                const auto raw_seat=std::uint16_t(m.word(unit+0x1fcu,2));
                const auto seat_index=std::int32_t(std::bit_cast<std::int16_t>(raw_seat));
                const auto seats=m.word(tag+0x1ccu);
                const auto seat=seats+std::uint32_t(seat_index)*0xb0u;
                m.word(0x18a068u,context,parent);
                m.word(0x18a06du,context+8u,seat+0x60u);
                const auto current_seat=m.word(unit+0x1fcu,2);
                m.word(0x18a077u,context+4u,current_seat,2);
                const auto flags=m.word(seat);
                if((flags&0x14u)==0x14u) {
                    const auto current_parent=m.word(context);
                    if(stats)++stats->base_position_calls;
                    (void)native_tu15_object_camera_origin(memory,current_parent,context+12u,fp,observe);
                }
            } else {
                const auto definition=m.word(unit),tags=m.word(0x4f0414u);
                const auto tag=m.word(tags+(definition&0xffffu)*16u+8u);
                m.word(0x18a0b3u,context+8u,tag+0xd4u);
            }
        }
        if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
