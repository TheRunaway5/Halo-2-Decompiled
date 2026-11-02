#include "halo2/native_tu15_constraint_handle_destruction.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t value{};
    for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(8u*n);return value;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t value,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
std::int32_t s16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
struct HandleRecord {std::uint32_t pool{},record{};};
HandleRecord handle_record(const SourcePhysicsContactMemory&m,std::uint32_t datum,bool rows_first){
    if(datum==0xffffffffu)return{};const auto pool=read(m,0x4f720cu),count=read(m,pool+0x38u),index=datum&0xffffu;
    if(!less(index,count))return{};std::uint32_t rows,stride;
    if(rows_first){rows=read(m,pool+0x44u);stride=read(m,pool+0x24u);}
    else{stride=read(m,pool+0x24u);rows=read(m,pool+0x44u);}
    const auto record=rows+index*stride,salt=read(m,record,2);
    if(!salt||s16(salt)!=(std::bit_cast<std::int32_t>(datum)>>16u))return{};
    return{pool,record};
}
std::uint32_t context_object(const SourcePhysicsContactMemory&m,std::uint32_t datum,std::uint32_t mask,
    NativeTu15ConstraintHandleDestructionStats*stats){ //BCDF0
    if(stats)++stats->object_queries;std::uint32_t record{};
    if(datum!=0xffffffffu){const auto pool=read(m,0x4ecbd0u),count=read(m,pool+0x38u),index=datum&0xffffu;
        if(less(index,count)){const auto stride=read(m,pool+0x24u),rows=read(m,pool+0x44u),candidate=rows+index*stride;
            const auto salt=read(m,candidate,2);
            if(salt&&s16(salt)==(std::bit_cast<std::int32_t>(datum)>>16u))record=candidate;
        }
    }
    if(!record)return 0;const auto kind=read(m,record+3u,1);
    if(!(mask&(1u<<(kind&31u))))return 0;if(stats)++stats->object_references;return read(m,record+8u);
}
void detach(SourcePhysicsContactMemory&m,std::uint32_t handle,const SourcePhysicsContactRecordObserver&o,
    NativeTu15ConstraintHandleDestructionStats*stats){ //17A420
    if(stats)++stats->detaches;const auto found=handle_record(m,handle,false);if(!found.record)return;
    const auto object=context_object(m,read(m,found.record+0x48u),0xffffffffu,stats);if(!object)return;
    const auto count=std::uint32_t(s16(read(m,object+0x11cu,2)))>>3u;
    const auto entries=object+std::uint32_t(s16(read(m,object+0x11eu,2)));
    for(std::uint32_t index=0;less(index,count);++index){const auto p=entries+index*8u;
        if(read(m,p+4u)==handle&&read(m,p,1)==2u){store(m,o,0x17a492u,p+4u,0xffffffffu);
            if(stats)++stats->detached_references;return;}
    }
}
void mark_link(SourcePhysicsContactMemory&m,std::uint32_t record,bool outer,const SourcePhysicsContactRecordObserver&o,
    NativeTu15ConstraintHandleDestructionStats*stats){
    const auto link=read(m,record+8u);if(link==0xffffffffu)return;
    const auto pool=read(m,0x4f9b5cu),rows=read(m,pool+0x44u),p=rows+(link&0xffffu)*24u+4u;
    store(m,o,outer?0x17a506u:0x17a3deu,p,read(m,p,1)|2u,1);
    store(m,o,outer?0x17a50fu:0x17a3e7u,record+8u,0xffffffffu);if(stats)++stats->links_marked;
}
void delete_event_lists(SourcePhysicsContactMemory&m,std::uint32_t handle,const SourcePhysicsContactRecordObserver&o,
    NativeTu15ConstraintHandleDestructionStats*stats){ //17A230
    const auto found=handle_record(m,handle,false);if(!found.record)return;
    const auto definition_datum=read(m,found.record+4u),definitions=read(m,0x4f0414u);
    const auto definition=read(m,definitions+(definition_datum&0xffffu)*16u+8u);
    if(!less(0u,read(m,definition+0xcu)))return;
    const auto pool=read(m,0x4f7208u);std::uint32_t index{};auto p=found.record+0x7cu;
    do{if(stats)++stats->event_lists;auto datum=read(m,p);
        if(datum!=0xffffffffu)do{const auto rows=read(m,pool+0x44u),next=read(m,rows+(datum&0xffffu)*60u+4u);
            native_tu15_data_array_delete(m,pool,datum);if(stats)++stats->event_deletions;
            datum=next;if(datum==0xffffffffu)break;
        }while(true);
        store(m,o,0x17a2ceu,p,0xffffffffu);const auto count=read(m,definition+0xcu);++index;p+=4u;
        if(!less(index,count))break;
    }while(true);
}
void cleanup_children(SourcePhysicsContactMemory&m,std::uint32_t record,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintHandleDestructionServices&s,const SourcePhysicsContactRecordObserver&o,
    NativeTu15ConstraintHandleDestructionStats*stats){ //17A2F0
    auto child=read(m,record+0x100u);
    if(child!=0xffffffffu)do{const auto pool=read(m,0x51df74u),rows=read(m,pool+0x44u),
        row=rows+(child&0xffffu)*84u,next=read(m,row+0x3cu);
        if(!s.unlink_child){if(stats)++stats->unbound_child_unlinks;
            throw std::runtime_error("Native TU1.5 handle destruction requires actual177FA0 child-link owner");}
        if(stats)++stats->child_unlinks;s.unlink_child(child,row,record+0xfcu,record+0x100u,fp);
        if(!s.delete_child){if(stats)++stats->unbound_child_deletions;
            throw std::runtime_error("Native TU1.5 handle destruction requires actual177020 child-release owner");}
        if(stats)++stats->child_deletions;s.delete_child(child,fp);child=next;
        if(child==0xffffffffu)break;
    }while(true);
    store(m,o,0x17a344u,record+0xfcu,0xffffffffu);store(m,o,0x17a34au,record+0x100u,0xffffffffu);
    for(unsigned n=0;n<16;++n){store(m,o,0x17a360u,record+0x110u+n*8u,0);
        store(m,o,0x17a366u,record+0x114u+n*8u,0xffffffffu);if(stats)++stats->tail_pairs;}
}
void destroy_now(SourcePhysicsContactMemory&m,std::uint32_t handle,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintHandleDestructionServices&s,const SourcePhysicsContactRecordObserver&o,
    NativeTu15ConstraintHandleDestructionStats*stats){ //17A380
    const auto found=handle_record(m,handle,true);if(!found.record)return;
    detach(m,handle,o,stats);mark_link(m,found.record,false,o,stats);
    native_tu15_cleanup_constraint_handle_tagged_values(m,found.record,fp,s.tagged_values,stats?&stats->tagged_values:nullptr);
    delete_event_lists(m,handle,o,stats);cleanup_children(m,found.record,fp,s,o,stats);
    native_tu15_data_array_delete(m,found.pool,handle);if(stats)++stats->row_deletions;
}
}
bool native_tu15_destroy_constraint_runtime_handle(SourcePhysicsContactMemory&m,std::uint32_t handle,
    NativeTu15PhysicsFrameFp&fp,const NativeTu15ConstraintHandleDestructionServices&s,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ConstraintHandleDestructionStats*stats){
    if(stats)++stats->calls;const auto found=handle_record(m,handle,false);
    if(!found.record){if(stats)++stats->invalid_handles;return true;}if(stats)++stats->valid_handles;
    detach(m,handle,observer,stats);mark_link(m,found.record,true,observer,stats);
    if(read(m,found.record+0xfcu)==0xffffffffu){
        destroy_now(m,handle,fp,s,observer,stats);if(stats)++stats->immediate_deletions;return true;
    }
    auto child=read(m,found.record+0x100u);
    if(child!=0xffffffffu){const auto pool=read(m,0x51df74u);
        do{const auto rows=read(m,pool+0x44u),row=rows+(child&0xffffu)*84u;
            store(m,observer,0x17a54du,row+0xcu,read(m,row+0xcu,2)&0xfffeu,2);if(stats)++stats->deactivated_children;
            child=read(m,row+0x3cu);if(child==0xffffffffu)break;
        }while(true);
    }
    store(m,observer,0x17a55cu,found.record+2u,read(m,found.record+2u,1)|0x44u,1);
    if(stats)++stats->deferred_deletions;return false;
}
} // namespace halo2
