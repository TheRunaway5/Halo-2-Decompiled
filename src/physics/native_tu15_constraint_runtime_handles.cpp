#include "halo2/native_tu15_constraint_runtime_handles.hpp"
#include "halo2/native_tu15_constraint_tag_cleanup.hpp"
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
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(value>>(8u*n));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
bool less(std::uint32_t a,std::uint32_t b){return std::bit_cast<std::int32_t>(a)<std::bit_cast<std::int32_t>(b);}
void cleanup_tagged_values(SourcePhysicsContactMemory&m,std::uint32_t record,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintRuntimeHandleServices&services,NativeTu15ConstraintRuntimeHandleStats*stats){
    if(stats)++stats->tag_cleanup_calls;const auto datum=read(m,record+4u),definitions=read(m,0x4f0414u);
    const auto definition=read(m,definitions+(datum&0xffffu)*16u+8u);
    if(!(read(m,definition)&32u)){if(stats)++stats->definition_skips;return;}
    if(!less(0u,read(m,definition+0x14u)))return;
    std::uint32_t row_index{},row_offset{};
    do{const auto row=read(m,definition+0x18u)+row_offset;if(stats)++stats->tag_rows;
        if(less(0u,read(m,row+0x18u))){std::uint32_t index{},offset{};
            do{const auto field=read(m,row+0x1cu)+offset;if(stats)++stats->tag_fields;
                if(read(m,field+0xcu)==0x7464746cu){const auto reference=read(m,field+0x10u);
                    if(reference!=0xffffffffu){const auto context=read(m,record+0x48u);
                        if(!services.publish_zero_tagged_value){if(stats)++stats->unbound_tag_value_calls;
                            throw std::runtime_error("Native TU1.5 runtime handle cleanup requires actualC6120 tagged-value owner");}
                        if(stats)++stats->tag_value_calls;services.publish_zero_tagged_value(reference,context,0u,fp);
                    }
                }
                const auto count=read(m,row+0x18u);++index;offset+=0x38u;if(!less(index,count))break;
            }while(true);
        }
        const auto count=read(m,definition+0x14u);++row_index;row_offset+=0x38u;if(!less(row_index,count))break;
    }while(true);
}
}
void native_tu15_cleanup_constraint_handle_tagged_values(SourcePhysicsContactMemory&m,
    std::uint32_t record,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintRuntimeHandleServices&services,NativeTu15ConstraintRuntimeHandleStats*stats){
    cleanup_tagged_values(m,record,fp,services,stats);
}
void native_tu15_release_constraint_runtime_handle(SourcePhysicsContactMemory&m,
    std::uint32_t handle,bool deletion_requested,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintRuntimeHandleServices&services,const SourcePhysicsContactRecordObserver&observer,
    NativeTu15ConstraintRuntimeHandleStats*stats){
    if(stats)++stats->calls;
    if(handle==0xffffffffu){if(stats)++stats->none_skips;return;}
    const auto pool=read(m,0x4f720cu),extent=read(m,pool+0x38u),index=handle&0xffffu;
    if(!less(index,extent)){if(stats)++stats->index_skips;return;}
    const auto rows=read(m,pool+0x44u),stride=read(m,pool+0x24u),record=rows+index*stride;
    const auto salt=read(m,record,2);
    const auto signed_salt=std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(salt)));
    const auto expected=std::bit_cast<std::int32_t>(handle)>>16u;
    if(!salt||signed_salt!=expected){if(stats)++stats->salt_skips;return;}
    if(stats)++stats->valid_handles;const auto link=read(m,record+8u);
    if(link!=0xffffffffu){const auto links=read(m,0x4f9b5cu),bank=read(m,links+0x44u),p=bank+(link&0xffffu)*24u+4u;
        store(m,observer,0x17a14bu,p,read(m,p,1)|2u,1);
        store(m,observer,0x17a154u,record+8u,0xffffffffu);if(stats)++stats->links_marked;
    }
    cleanup_tagged_values(m,record,fp,services,stats);
    if(read(m,record+2u,1)&2u){
        if(deletion_requested)store(m,observer,0x17a172u,record+2u,read(m,record+2u,1)|0x10u,1);
        else store(m,observer,0x17a17eu,record+2u,read(m,record+2u,1)&0xefu,1);
        store(m,observer,deletion_requested?0x17a176u:0x17a182u,record+2u,read(m,record+2u,1)|4u,1);
        if(stats)++stats->retained_marks;return;
    }
    if(!services.destroy_handle){if(stats)++stats->unbound_destroy_calls;
        throw std::runtime_error("Native TU1.5 runtime handle cleanup requires actual17A4A0 destruction owner");}
    if(stats)++stats->destroy_calls;services.destroy_handle(handle,fp);
}
void native_tu15_clear_constraint_runtime_handles(SourcePhysicsContactMemory&m,
    std::uint32_t instance,bool clear_links,bool clear_handles,NativeTu15PhysicsFrameFp&fp,
    const NativeTu15ConstraintRuntimeHandleServices&services,const SourcePhysicsContactRecordObserver&observer,
    NativeTu15ConstraintRuntimeHandleStats*stats){
    if(stats)++stats->clear_calls;
    if(clear_links){auto link=read(m,instance+0x30u);const auto links=read(m,0x4f9b5cu);
        if(link!=0xffffffffu){const auto bank=read(m,links+0x44u),p=bank+(link&0xffffu)*24u+4u;
            store(m,observer,0x230a80u,p,read(m,p,1)|2u,1);store(m,observer,0x230a89u,instance+0x30u,0xffffffffu);
            if(stats)++stats->clear_links;}
        link=read(m,instance+0x34u);
        if(link!=0xffffffffu){const auto bank=read(m,links+0x44u),p=bank+(link&0xffffu)*24u+4u;
            store(m,observer,0x230a9eu,p,read(m,p,1)|2u,1);store(m,observer,0x230aa7u,instance+0x34u,0xffffffffu);
            if(stats)++stats->clear_links;}
    }
    if(clear_handles)for(unsigned n=0;n<2;++n){const auto p=instance+0x38u+n*4u,handle=read(m,p);
        if(handle!=0xffffffffu){native_tu15_release_constraint_runtime_handle(m,handle,true,fp,services,observer,stats);
            store(m,observer,n?0x230ad2u:0x230ac1u,p,0xffffffffu);if(stats)++stats->clear_handles;}
    }
}
bool native_tu15_constraint_has_runtime_handles(const SourcePhysicsContactMemory&m,
    std::uint32_t instance,NativeTu15ConstraintRuntimeHandleStats*stats){
    if(stats)++stats->presence_calls;
    if(read(m,instance+0x30u)!=0xffffffffu)return true;
    if(read(m,instance+0x34u)!=0xffffffffu)return true;
    if(read(m,instance+0x38u)!=0xffffffffu)return true;
    return read(m,instance+0x3cu)!=0xffffffffu;
}
} // namespace halo2
