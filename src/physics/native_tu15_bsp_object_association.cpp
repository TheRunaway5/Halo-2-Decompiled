#include "halo2/native_tu15_bsp_object_association.hpp"
#include "halo2/native_tu15_retained_store_observation.hpp"
#include <bit>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned n=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(n));std::uint32_t value{};
        for(unsigned k=0;k<n;++k)value|=std::to_integer<unsigned>(b[k])<<(8u*k);return value;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned n=4){
        // These authored mask/object fields belong to retained owners. An
        // aliased native local must never conceal their publication.
        if(pc==0x2190cfu||pc==0x219137u||pc==0x2191a3u||pc==0x2191acu||pc==0x2192bcu||pc==0x2192c7u)
            if(const auto*domain=dynamic_cast<const NativeTu15RetainedStoreObservationDomain*>(&memory_))
                domain->require_retained_extent(p,n);
        std::array<std::byte,4>b{};for(unsigned k=0;k<n;++k)b[k]=std::byte(value>>(8u*k));
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        const bool observed=native_tu15_observes_retained_store(memory_,observe_,pc,p,n);
        if(observed){event.before.resize(n);memory_.read(p,event.before);event.written.assign(b.begin(),b.begin()+n);}
        memory_.write(p,std::span(b).first(n));if(observed)observe_(event);
    }
};
std::int32_t signed_word(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
void initialize_reset_scan(Memory&m,std::uint32_t p){
    m.word(0x21915bu,p+0xcu,0x86868686u);m.word(0x219163u,p,0x80u);
    m.word(0x21916bu,p+4u,0,1);m.word(0x21916fu,p+6u,0,2);m.word(0x219174u,p+8u,0xffffffffu);
}
void initialize_association_scan(Memory&m,std::uint32_t p){
    m.word(0x219211u,p+0xcu,0x86868686u);m.word(0x219219u,p,0x80u);
    m.word(0x219221u,p+4u,0,1);m.word(0x219226u,p+6u,0,2);m.word(0x21922bu,p+8u,0xffffffffu);
}
}
std::uint32_t native_tu15_next_scanned_object(SourcePhysicsContactMemory&memory,
    NativeTu15ObjectScanState scan,const SourcePhysicsContactRecordObserver&observe){
    Memory m(memory,observe);const auto p=scan.identity,pool=m.word(0x4ecbd0u);
    const auto rows=m.word(pool+0x44u),cursor=m.word(p+6u,2),limit=m.word(pool+0x38u);
    return native_tu15_scan_objects(memory,{scan,rows,cursor,limit},observe).object;
}
void native_tu15_reset_bsp_object_associations(SourcePhysicsContactMemory&memory,
    NativeTu15ObjectScanState scan,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15BspObjectAssociationStats*stats){
    Memory m(memory,observe);initialize_reset_scan(m,scan.identity);
    const auto next=[&]{if(stats)++stats->iterator_calls;return native_tu15_next_scanned_object(memory,scan,observe);};
    if(next()){
        const auto pool=m.word(0x4ecbd0u);
        do{
            const auto datum=m.word(scan.identity+8u),rows=m.word(pool+0x44u);
            const auto object=m.word(rows+(datum&0xffffu)*12u+8u);
            m.word(0x2191a3u,object+0x1dau,0xffffu,2);m.word(0x2191acu,object+0x1dcu,0,2);
            if(stats)++stats->reset_objects;
        }while(next());
    }
}
void native_tu15_apply_bsp_object_associations(SourcePhysicsContactMemory&memory,
    NativeTu15BspObjectAssociationScratch scratch,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15BspObjectAssociationStats*stats){
    native_tu15_reset_bsp_object_associations(memory,scratch.reset_scan,observe,stats);
    Memory m(memory,observe);const auto scenario=m.word(0x4ecc18u),records=m.word(scenario+0x220u);
    const auto initial_count=m.word(records);if(signed_word(initial_count)<=0)return;
    std::uint32_t index{},offset{};
    do{
        const auto record=m.word(records+4u)+offset;initialize_association_scan(m,scratch.association_scan.identity);
        const auto next=[&]{if(stats)++stats->iterator_calls;return native_tu15_next_scanned_object(memory,scratch.association_scan,observe);};
        if(next()){
            const auto pool=m.word(0x4ecbd0u),rows=m.word(pool+0x44u);
            const auto definition=m.word(record),variant=m.word(record+7u,1);
            do{
                const auto datum=m.word(scratch.association_scan.identity+8u);
                const auto object=m.word(rows+(datum&0xffffu)*12u+8u);
                const auto first=m.word(object+0xaau,1),wanted=m.word(record+6u,1);
                const auto second=m.word(object+0xabu,1),actual_definition=m.word(object+0xa4u);
                bool matches=first==wanted&&second==variant&&actual_definition==definition;
                if(matches&&!variant){const auto part=m.word(record+4u,2),actual=m.word(object+0xa8u,2);matches=part==actual;}
                if(matches){
                    const auto bsp=m.word(record+8u,2);m.word(0x2192bcu,object+0x1dau,bsp,2);
                    const auto cluster=m.word(record+0xau,2);m.word(0x2192c7u,object+0x1dcu,cluster,2);
                    if(stats)++stats->matched_objects;break;
                }
            }while(next());
        }
        const auto count=m.word(records);++index;offset+=12u;
        if(stats)++stats->association_records;
        if(signed_word(index)>=signed_word(count))break;
    }while(true);
}
void native_tu15_exclude_noncollidable_scenery(SourcePhysicsContactMemory&memory,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15BspObjectAssociationStats*stats){
    Memory m(memory,observe);const auto scenario=m.word(0x4ecc18u),initial_count=m.word(scenario+0x5cu);
    if(signed_word(initial_count)<=0)return;const auto mask=m.word(0x505e7cu);
    std::uint32_t index{},offset{};
    do{
        const auto objects=m.word(scenario+0x60u);if(m.word(objects+offset+0x18u,1)&8u){
            const auto bit=1u<<(index&31u),destination=mask+(std::uint32_t(signed_word(index)>>5u))*4u;
            const auto previous=m.word(destination);m.word(0x219137u,destination,previous&~bit);
            if(stats)++stats->excluded_objects;
        }
        const auto count=m.word(scenario+0x5cu);++index;offset+=0x24u;
        if(signed_word(index)>=signed_word(count))break;
    }while(true);
}
void native_tu15_initialize_bsp_object_associations(SourcePhysicsContactMemory&memory,
    NativeTu15BspObjectAssociationScratch scratch,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15BspObjectAssociationStats*activity){
    NativeTu15BspObjectAssociationStats local;auto&stats=activity?*activity:local;++stats.attempts;
    try{
        Memory m(memory,observe);const auto destination=m.word(0x505e7cu);
        for(unsigned n=0;n<16u;++n)m.word(0x2190cfu,destination+n*4u,0xffffffffu);
        native_tu15_apply_bsp_object_associations(memory,scratch,observe,&stats);
        native_tu15_exclude_noncollidable_scenery(memory,observe,&stats);++stats.completed;
    }catch(...){++stats.refusals;throw;}
}
} // namespace halo2
