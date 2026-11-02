#include "halo2/native_tu15_contact_point_storage.hpp"
#include "halo2/native_tu15_array_reserve.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&m_;const SourcePhysicsContactRecordObserver&o_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):m_(m),o_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const {
        std::array<std::byte,4>b{};m_.read(p,std::span(b).first(n));std::uint32_t v{};
        for(unsigned k=0;k<n;++k)v|=std::to_integer<unsigned>(b[k])<<(8u*k);return v;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned n=4){
        std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(v>>(8u*k));
        SourcePhysicsContactRecordStore e{pc,p,{},{}};
        if(o_){e.before.resize(n);m_.read(p,e.before);}
        m_.write(p,std::span(b).first(n));
        if(o_){e.written.assign(b.begin(),b.begin()+n);o_(e);}
    }
};
bool signed_less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
} // namespace

std::uint32_t native_tu15_allocate_contact_point_storage(SourcePhysicsContactMemory&memory,
    std::uint32_t collector,std::uint32_t point_cell,std::uint32_t property_cell,
    NativeTu15ContactPointStorageScratch scratch,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ContactPointStorageServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ContactPointStorageStats*stats){
    if(stats)++stats->attempts;
    try {
        if(scratch.info&3u)throw std::runtime_error("Native TU1.5 contact point storage requires aligned info DATA");
        Memory m(memory,observe);
        const auto old_properties=m.word(collector+0x94u);
        m.word(0x325fa4u,scratch.info,old_properties*48u+0xb0u);
        const auto special=old_properties==1u;
        m.word(0x325fbcu,scratch.info+4u,special?80u:48u);
        m.word(0x325fc0u,scratch.info+8u,special?8u:4u);
        const auto island=m.word(collector+0x14u);
        m.word(0x325fcdu,scratch.info+12u,special?2u:1u);
        const auto table=m.word(island),owner=m.word(table+8u);
        if(stats)++stats->metadata_updates;
        if(!services.add_info)throw std::runtime_error("Native TU1.5 contact point storage requires island metadata owner");
        services.add_info(island,{owner},collector,scratch.info,fp);
        m.word(0x325fd7u,collector+0x50u,m.word(collector+0x50u)|6u);
        const auto old_points=m.word(collector+0x38u);
        const auto point_capacity=m.word(collector+0x3cu)&0x7fffffffu;
        const auto index=m.word(collector+0x38u),needed_points=old_points+1u;
        if(signed_less(point_capacity,needed_points)){
            const auto doubled=point_capacity+point_capacity;
            const auto capacity=signed_less(needed_points,doubled)?doubled:needed_points;
            native_tu15_reserve_array(memory,{collector+0x34u},capacity,32u,fp,services.allocation,observe);
            if(stats)++stats->point_reservations;
        }
        m.word(0x32600fu,collector+0x38u,needed_points);
        const auto point_data=m.word(collector+0x34u);
        const auto point=point_data+old_points*32u;
        const auto property_count=m.word(collector+0x94u),needed_properties=property_count+1u;
        const auto tail=property_count-index;
        const auto property_capacity=m.word(collector+0x98u)&0x7fffffffu;
        if(signed_less(property_capacity,needed_properties)){
            const auto doubled=property_capacity+property_capacity;
            const auto capacity=signed_less(needed_properties,doubled)?doubled:needed_properties;
            native_tu15_reserve_array(memory,{collector+0x90u},capacity,28u,fp,services.allocation,observe);
            if(stats)++stats->property_reservations;
        }
        const auto property_data=m.word(collector+0x90u),offset=index*28u;
        if(!signed_less(tail-1u,0u)){
            auto destination=property_data+offset+tail*28u;
            auto remaining=tail;
            do {
                const auto source=destination-28u;
                for(unsigned word=0;word<7;++word){
                    const auto value=m.word(source+word*4u);
                    m.word(0x32609au,destination+word*4u,value);
                }
                destination-=28u;
                if(stats)++stats->shifted_property_records;
            }while(--remaining);
        }
        m.word(0x3260b2u,collector+0x94u,needed_properties);
        const auto property=m.word(collector+0x90u)+offset;
        m.word(0x3260beu,property,0u);
        m.word(0x3260c1u,property+4u,0u);
        m.word(0x3260c4u,property+0x18u,0u,2);
        if(signed_less(0u,index)&&m.word(property-4u,2)==0u)m.word(0x3260d0u,property+0x18u,1u,2);
        m.word(0x3260deu,point_cell,point);
        m.word(0x3260e8u,property_cell,property);
        const auto id=native_tu15_allocate_contact_point_id(memory,{collector+0x20u},index,fp,services.allocation,observe,stats?&stats->ids:nullptr);
        if(stats)++stats->completed;
        return id;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
