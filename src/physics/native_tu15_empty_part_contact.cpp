#include "halo2/native_tu15_empty_part_contact.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
void native_tu15_release_empty_part_contact(SourcePhysicsContactMemory&memory,
    std::uint32_t part_slot,std::int32_t part,std::uint32_t component,
    std::uint32_t removed_result,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe){
    const auto read=[&](std::uint32_t address,unsigned width=4){
        std::array<std::byte,4>bytes{};memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);return value;
    };
    // Source loads all three default-vector words before the count, including
    // on its zero-count branch. Preserve those accesses and fault prefixes.
    const auto defaults=read(0x474d04u);
    const auto x=read(defaults),y=read(defaults+4u),z=read(defaults+8u);
    (void)x;(void)y;(void)z;
    const auto count=read(component+0x8cu);
    if(std::bit_cast<std::int32_t>(count)>0){
        const auto rows=read(component+0x88u);
        for(std::uint32_t index=0;index<count;++index){
            const auto row=rows+index*72u;
            const auto signed_part=std::bit_cast<std::int8_t>(std::uint8_t(read(row+0x45u,1)));
            if(signed_part==part&&(read(row+0x44u,1)&0x20u))
                throw std::runtime_error("Native TU1.5 part contact requires its remaining positive material/conveyor branch");
        }
    }
    native_tu15_clear_part_contact_response(memory,part_slot,removed_result,fp,allocation,observe);
}
void native_tu15_clear_part_contact_response(SourcePhysicsContactMemory&memory,
    std::uint32_t part_slot,std::uint32_t removed_result,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ArrayGrowthServices&allocation,const SourcePhysicsContactRecordObserver&observe){
    std::array<std::byte,4>body_bytes{};memory.read(part_slot+0x40u,body_bytes);std::uint32_t body{};
    for(unsigned n=0;n<4;++n)body|=std::to_integer<unsigned>(body_bytes[n])<<(n*8u);
    native_tu15_replace_part_contact_property(memory,body,0u,removed_result,fp,allocation,observe);
    const auto destination=part_slot+0x30u;
    if(destination&15u)throw std::runtime_error("Native TU1.5 empty part contact requires source MOVAPS alignment");
    const std::array<std::byte,16>zero{};
    SourcePhysicsContactRecordStore vector_event{0x1e0b60u,destination,{},{}};
    if(observe){vector_event.before.resize(16);memory.read(destination,vector_event.before);
        vector_event.written.assign(zero.begin(),zero.end());}
    memory.write(destination,zero);if(observe)observe(vector_event);
    const std::array byte{std::byte{0}};
    SourcePhysicsContactRecordStore flag_event{0x1e0b64u,part_slot+0x44u,{},{}};
    if(observe){flag_event.before.resize(1);memory.read(flag_event.address,flag_event.before);flag_event.written.assign(byte.begin(),byte.end());}
    memory.write(flag_event.address,byte);if(observe)observe(flag_event);
}
} // namespace halo2
