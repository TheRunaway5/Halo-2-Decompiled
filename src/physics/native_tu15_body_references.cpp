#include "halo2/native_tu15_body_references.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;
    const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o)
        :memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t address)const{
        std::array<std::byte,4>raw{};memory_.read(address,raw);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value){
        std::array<std::byte,4>raw{};for(unsigned n=0;n<4;++n)raw[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(4);memory_.read(address,event.before);
            event.written.assign(raw.begin(),raw.end());}
        memory_.write(address,raw);if(observe_)observe_(event);
    }
};
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
struct Cohort {std::uint32_t offset,copy_site,count_site;};
Cohort descriptor(NativeTu15BodyReferenceCohort cohort){
    switch(cohort){
        case NativeTu15BodyReferenceCohort::first:return{0x7cu,0x3174d9u,0x317504u};
        case NativeTu15BodyReferenceCohort::second:return{0x88u,0x3173f9u,0x317422u};
        case NativeTu15BodyReferenceCohort::third:return{0x94u,0x317469u,0x317492u};
    }
    throw std::runtime_error("Native TU1.5 body reference cohort is not a source owner");
}
}
void native_tu15_compact_body_reference_cohort(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsBody body,NativeTu15BodyReferenceCohort kind,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15BodyReferenceServices&s,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15BodyReferenceStats*stats){
    Memory m(memory,observe);const auto cohort=descriptor(kind);
    const auto header=body.identity+cohort.offset,count=m.word(header+4u);
    if(stats)++stats->cohorts;
    std::uint32_t gap{};
    if(less(0u,count)){
        // Original first scan caches this data pointer; the second scan below
        // rereads the field for every entry, including across observer writes.
        const auto first=m.word(header);
        do{
            if(!m.word(first+gap*4u))break;
            ++gap;
        }while(less(gap,count));
    }
    auto compacted=gap;auto index=gap+1u;
    while(less(index,count)){
        const auto data=m.word(header),value=m.word(data+index*4u);
        if(value){m.word(cohort.copy_site,data+compacted*4u,value);++compacted;
            if(stats)++stats->moved_entries;}
        ++index;
    }
    const auto capacity=m.word(header+8u)&0x7fffffffu;
    if(less(capacity,compacted)){
        const auto doubled=capacity+capacity;
        const auto requested=less(compacted,doubled)?doubled:compacted;
        if(!s.reserve_array)throw std::runtime_error("Native TU1.5 body references require actual2E31E0 reserve");
        if(stats)++stats->reserves;s.reserve_array({header},requested,4u,fp);
    }
    m.word(cohort.count_site,header+4u,compacted);
}
void native_tu15_compact_body_references(SourcePhysicsContactMemory&memory,
    NativeTu15PhysicsBody body,NativeTu15PhysicsFrameFp&fp,const NativeTu15BodyReferenceServices&s,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15BodyReferenceStats*stats){
    if(stats)++stats->calls;
    native_tu15_compact_body_reference_cohort(memory,body,NativeTu15BodyReferenceCohort::first,fp,s,observe,stats);
    native_tu15_compact_body_reference_cohort(memory,body,NativeTu15BodyReferenceCohort::second,fp,s,observe,stats);
    native_tu15_compact_body_reference_cohort(memory,body,NativeTu15BodyReferenceCohort::third,fp,s,observe,stats);
}
} // namespace halo2
