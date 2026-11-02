#include "halo2/native_tu15_model_constraint_removal.hpp"
#include "halo2/native_tu15_data_array_delete.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory&m,std::uint32_t p,unsigned width=4){
    std::array<std::byte,4>b{};m.read(p,std::span(b).first(width));std::uint32_t v{};
    for(unsigned n=0;n<width;++n)v|=std::to_integer<unsigned>(b[n])<<(8u*n);return v;
}
void store(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
    std::uint32_t pc,std::uint32_t p,std::uint32_t v,unsigned width=4){
    std::array<std::byte,4>b{};for(unsigned n=0;n<width;++n)b[n]=std::byte(v>>(8u*n));
    SourcePhysicsContactRecordStore event{pc,p,{},{}};
    if(o){event.before.resize(width);m.read(p,event.before);event.written.assign(b.begin(),b.begin()+width);}
    m.write(p,std::span(b).first(width));if(o)o(event);
}
std::int32_t s32(std::uint32_t v){return std::bit_cast<std::int32_t>(v);}
std::int32_t s16(std::uint32_t v){return std::bit_cast<std::int16_t>(std::uint16_t(v));}
}
void native_tu15_remove_model_constraint(SourcePhysicsContactMemory&m,std::uint32_t datum,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15ModelConstraintRemovalServices&services,
    const SourcePhysicsContactRecordObserver&observer,NativeTu15ModelConstraintRemovalStats*stats){
    if(stats)++stats->calls;
    const auto instances=read(m,read(m,0x52d68cu)+0x44u);
    const auto instance=instances+(datum&0xffffu)*160u;
    const auto first=read(m,instance+0x24u);
    const auto components=read(m,0x52be5cu),rows=read(m,components+0x44u);
    const auto cached=read(m,instance+3u,1),component=rows+(first&0xffffu)*160u;
    if(cached!=0xffu){
        const auto index=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(cached))));
        store(m,observer,0x22d0e6u,0x50ec14u+index*4u,0xffffffffu);
        if(stats)++stats->cached_slots_cleared;
    }
    auto link=read(m,instance+0x30u);const auto link_pool=read(m,0x4f9b5cu);
    if(link!=0xffffffffu){const auto bank=read(m,link_pool+0x44u),p=bank+(link&0xffffu)*24u+4u;
        store(m,observer,0x22d10cu,p,read(m,p,1)|2u,1);store(m,observer,0x22d115u,instance+0x30u,0xffffffffu);
        if(stats)++stats->links_marked;}
    link=read(m,instance+0x34u);
    if(link!=0xffffffffu){const auto bank=read(m,link_pool+0x44u),p=bank+(link&0xffffu)*24u+4u;
        store(m,observer,0x22d12au,p,read(m,p,1)|2u,1);store(m,observer,0x22d133u,instance+0x34u,0xffffffffu);
        if(stats)++stats->links_marked;}
    for(unsigned n=0;n<2;++n){const auto offset=0x38u+n*4u,handle=read(m,instance+offset);
        if(handle==0xffffffffu)continue;
        if(!services.release_runtime_handle){if(stats)++stats->unbound_handle_releases;
            throw std::runtime_error("Native TU1.5 model constraint removal requires actual17A0F0 handle lifetime owner");}
        if(stats)++stats->handle_releases;services.release_runtime_handle(handle,true,fp);
        store(m,observer,n?0x22d15au:0x22d144u,instance+offset,0xffffffffu);
    }
    if(read(m,instance+8u,2)!=0){
        if(read(m,instance+0xdu,1)!=0xffu){
            native_tu15_release_constraint_reference(m,instance,datum,fp,depth,services.reference,observer,
                stats?&stats->reference:nullptr);
        }else{
            if(s32(read(m,component+0x8cu))>0){
                std::uint32_t index{},offset{};
                do{const auto row=read(m,component+0x88u)+offset;
                    if(stats)++stats->local_rows;
                    if(read(m,row+8u)==datum){
                        native_tu15_release_constraint_reference(m,instance,datum,fp,depth,services.reference,observer,
                            stats?&stats->reference:nullptr);
                        store(m,observer,0x22d1abu,row+8u,0xffffffffu);if(stats)++stats->local_references;
                    }
                    const auto count=read(m,component+0x8cu);++index;offset+=0x48u;
                    if(s32(index)>=s32(count))break;
                }while(true);
            }
            const auto initial=read(m,component+0x80u);
            if(s32(initial)>0){std::uint32_t index{},offset{};
                do{auto bank=read(m,component+0x7cu);if(stats)++stats->global_rows;
                    if(read(m,bank+offset+4u)==datum){
                        if(read(m,instance+8u,2)==1u){const auto state=read(m,instance+0x10u,1);
                            if(state==1u){
                                const auto slots=native_tu15_constraint_body_slots(m,instance,datum,stats?&stats->slots:nullptr);
                                const auto scalar=read(m,instance+0x14u);
                                native_tu15_update_constraint_cached_scalar(m,instance,scalar,fp,depth,observer,stats?&stats->cache:nullptr);
                                if(!services.reference.remove_runtime_constraint){if(stats)++stats->unbound_runtime_removals;
                                    throw std::runtime_error("Native TU1.5 model constraint removal requires actual22FEA0 runtime lifetime owner");}
                                if(stats)++stats->runtime_removals;services.reference.remove_runtime_constraint(instance,datum,slots,fp);
                            }
                        }
                        store(m,observer,0x22d238u,instance+8u,read(m,instance+8u,2)-1u,2);
                        bank=read(m,component+0x7cu);store(m,observer,0x22d23fu,bank+offset+4u,0xffffffffu);
                        if(stats)++stats->global_references;
                    }
                    const auto count=read(m,component+0x80u);++index;offset+=12u;
                    if(s32(index)>=s32(count))break;
                }while(true);
            }
        }
    }
    const auto list=read(m,component+0x20u),list_pool=read(m,0x52d690u),list_bank=read(m,list_pool+0x44u);
    const auto list_row=list_bank+(list&0xffffu)*64u;
    const auto original_count=s16(read(m,list_row+2u,2));std::uint32_t index{};
    if(original_count>0){auto p=list_row+4u;
        do{if(stats)++stats->list_rows;
            if(read(m,p)==datum){const auto last=read(m,list_row+std::uint32_t(original_count)*4u);
                store(m,observer,0x22d299u,list_row+index*4u+4u,last);
                store(m,observer,0x22d29du,list_row+2u,read(m,list_row+2u,2)-1u,2);
                if(stats)++stats->list_removals;break;
            }
            const auto count=s16(read(m,list_row+2u,2));++index;p+=4u;
            if(s32(index)>=count)break;
        }while(true);
    }
    const auto current_list=read(m,component+0x20u),current_bank=read(m,list_pool+0x44u);
    if(read(m,current_bank+(current_list&0xffffu)*64u+2u,2)==0){
        native_tu15_data_array_delete(m,list_pool,current_list);
        store(m,observer,0x22d2bfu,component+0x20u,0xffffffffu);if(stats)++stats->list_deletions;
    }
    const auto instance_pool=read(m,0x52d68cu);native_tu15_data_array_delete(m,instance_pool,datum);
    if(stats)++stats->instance_deletions;
}
} // namespace halo2
