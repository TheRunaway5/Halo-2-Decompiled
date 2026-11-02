#include "halo2/native_tu15_object_publication.hpp"
namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned k=0;k<width;++k)value|=std::to_integer<unsigned>(b[k])<<(k*8u);
    return value;
}
std::uint32_t header(std::uint32_t rows,std::uint32_t datum){return rows+(datum&0xffffu)*12u;}
void mark_header(SourcePhysicsContactMemory&m,std::uint32_t p,
    const SourcePhysicsContactRecordObserver&observe){
    const auto value=word(m,p,1)|4u;const std::array bytes{std::byte(value)};
    SourcePhysicsContactRecordStore event{0xbda93u,p,{},{}};
    if(observe){event.before.resize(1);m.read(p,event.before);event.written.assign(bytes.begin(),bytes.end());}
    m.write(p,bytes);if(observe)observe(event);
}
void wake(SourcePhysicsContactMemory&m,NativeTu15PhysicsObject object,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15ObjectPublicationStats*stats){
    if(stats)++stats->ancestor_wakes;
    native_tu15_wake_object_ancestors(m,object.datum,observe);
}
}
std::uint32_t native_tu15_publication_root(const SourcePhysicsContactMemory&m,
    NativeTu15PhysicsObject object){
    if(object.datum==0xffffffffu)return object.datum;
    const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u);
    std::uint32_t result{};
    do{result=object.datum;const auto data=word(m,header(rows,object.datum)+8u);
        object.datum=word(m,data+0x14u);
    }while(object.datum!=0xffffffffu);
    return result;
}
void native_tu15_mark_publication_tree(SourcePhysicsContactMemory&m,
    NativeTu15PhysicsObject object,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectPublicationStats*stats){
    if(stats)++stats->tree_calls;
    const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u),p=header(rows,object.datum);
    const auto data=word(m,p+8u),kind=word(m,p+3u,1);
    if((1u<<(kind&31u))&0x1003u)wake(m,object,observe,stats);
    auto child=word(m,data+0x10u);
    while(child!=0xffffffffu){
        const auto current_pool=word(m,0x4ecbd0u),current_rows=word(m,current_pool+0x44u);
        const auto retained_header=header(current_rows,child);
        native_tu15_mark_publication_tree(m,{child},observe,stats);
        const auto returned_data=word(m,retained_header+8u);child=word(m,returned_data+12u);
    }
}
void native_tu15_publish_object_update(SourcePhysicsContactMemory&m,
    NativeTu15PhysicsObject object,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15ObjectPublicationStats*stats){
    if(stats)++stats->calls;
    const auto pool=word(m,0x4ecbd0u),rows=word(m,pool+0x44u);
    auto p=header(rows,object.datum);
    if(word(m,p+2u,1)&0x80u){
        if(stats)++stats->root_queries;
        object.datum=native_tu15_publication_root(m,object);p=header(rows,object.datum);
    }
    if(word(m,p+2u,1)&1u){
        wake(m,object,observe,stats);mark_header(m,p+2u,observe);
        if(stats)++stats->marked_headers;
    }
    native_tu15_mark_publication_tree(m,object,observe,stats);
}
} // namespace halo2
