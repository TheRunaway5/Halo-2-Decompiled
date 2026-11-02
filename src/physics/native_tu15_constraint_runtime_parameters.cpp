#include "halo2/native_tu15_constraint_runtime_parameters.hpp"
#include "halo2/native_tu15_data_array_allocate.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
}
bool native_tu15_update_constraint_runtime_parameters(SourcePhysicsContactMemory&m,std::uint32_t handle,
    NativeTu15ConstraintRuntimeParameterUpdate input,const SourcePhysicsContactRecordObserver&observer,
    NativeTu15ConstraintRuntimeParameterStats*stats){
    if(stats)++stats->calls;if(handle==0xffffffffu){if(stats)++stats->none_skips;return false;}
    const auto pool=read(m,0x4f720cu),extent=read(m,pool+0x38u),index=handle&0xffffu;
    if(!less(index,extent)){if(stats)++stats->index_skips;return false;}
    const auto stride=read(m,pool+0x24u),rows=read(m,pool+0x44u),record=rows+index*stride;
    const auto salt=read(m,record,2);const auto actual=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(salt)));
    if(!salt||actual!=(std::bit_cast<std::int32_t>(handle)>>16u)){if(stats)++stats->salt_skips;return false;}
    if(stats)++stats->valid_handles;const auto definition_datum=read(m,record+4u),definitions=read(m,0x4f0414u);
    auto definition=read(m,definitions+(definition_datum&0xffffu)*16u+8u);
    store(m,observer,0x17df25u,record+3u,read(m,record+3u,1)|2u,1);
    if(input.position){for(unsigned n=0;n<3;++n)
        store(m,observer,n==0?0x17df36u:n==1?0x17df3bu:0x17df41u,record+0x30u+n*4u,read(m,input.position+n*4u));
        if(stats)++stats->positions;}
    if(input.bounds){store(m,observer,0x17df4eu,record+0x74u,read(m,input.bounds));
        store(m,observer,0x17df54u,record+0x78u,read(m,input.bounds+4u));if(stats)++stats->bounds;}
    store(m,observer,0x17df59u,record+0x6cu,input.primary_scalar_bits);
    store(m,observer,0x17df64u,record+0x70u,input.secondary_scalar_bits);
    if(input.frame&&less(0u,read(m,definition+0xcu))){
        const auto nodes=read(m,0x4f7208u),one=read(m,0x46a0d0u);std::uint32_t list{},offset{};
        do{
            if(stats)++stats->frame_lists;auto node=read(m,record+0x7cu+offset);
            if(node!=0xffffffffu){
                do{const auto bank=read(m,nodes+0x44u),row=bank+(node&0xffffu)*0x3cu,destination=row+8u;
                    node=read(m,row+4u);for(unsigned n=0;n<13;++n)
                        store(m,observer,0x17dfbfu,destination+n*4u,read(m,input.frame+n*4u));
                    store(m,observer,0x17dfc1u,destination,one);if(stats)++stats->frame_nodes;
                }while(node!=0xffffffffu);
            }
            const auto count=read(m,definition+0xcu);++list;offset+=4u;if(!less(list,count))break;
        }while(true);
    }
    return true;
}
void native_tu15_set_constraint_runtime_link_flag(SourcePhysicsContactMemory&m,std::uint32_t link,bool enabled,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintRuntimeParameterStats*stats){
    if(stats)++stats->link_flag_updates;const auto pool=read(m,0x4f9b5cu),rows=read(m,pool+0x44u),p=rows+(link&0xffffu)*24u+4u;
    const auto before=read(m,p,1);store(m,observer,enabled?0x18d52du:0x18d533u,p,enabled?before|0x10u:before&0xefu,1);
}
std::uint32_t native_tu15_create_constraint_runtime_link(SourcePhysicsContactMemory&m,std::uint32_t handle,
    std::uint32_t reference,const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintRuntimeParameterStats*stats){
    if(stats)++stats->link_creation_calls;if(handle==0xffffffffu){if(stats)++stats->link_creation_none;return 0xffffffffu;}
    const auto pool=read(m,0x4f9b5cu);const auto result=native_tu15_data_array_allocate(m,pool);
    if(result==0xffffffffu){if(stats)++stats->allocation_failures;return result;}
    const auto rows=read(m,pool+0x44u),row=rows+(result&0xffffu)*24u;
    store(m,observer,0x18d4e0u,row+3u,2u,1);store(m,observer,0x18d4e4u,row+2u,3u,1);
    store(m,observer,0x18d4e8u,row+0x10u,reference);store(m,observer,0x18d4ebu,row+4u,0u,2);
    store(m,observer,0x18d4f1u,row+5u,read(m,row+5u,1)|1u,1);store(m,observer,0x18d4f5u,row+8u,0u);
    store(m,observer,0x18d4fau,row+0xcu,handle);store(m,observer,0x18d4fdu,row+6u,0xffffu,2);
    store(m,observer,0x18d503u,row+4u,read(m,row+4u,1)|1u,1);if(stats)++stats->links_created;return result;
}
} // namespace halo2
